//
// Created by sebastian on 30.07.25.
//

#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <unordered_set>

#include "capi/selection/DOTWriter.h"
#include "capi/selection/Demangle.h"
#include "capi/selection/DriverUtils.h"
#include "capi/selection/FunctionFilter.h"
#include "capi/selection/MeasurementConfigIO.h"
#include "capi/selection/SCC.h"
#include "capi/selection/SelectorBuilder.h"
#include "capi/selection/SelectorGraph.h"
#include "capi/selection/SpecParser.h"
#include "capi/selection/StatementCountAnalysis.h"
#include "capi/selection/metadata/CaPIMD.h"
#include "capi/support/Logging.h"
#include "capi/support/Timer.h"
#include "capi/symbol_retriever/SymbolRetriever.h"
#include "capi_version.h"

// MetaCG
#include "io/MCGReader.h"

CAPI_DEFINE_VERBOSITY(LOG_STATUS)

using namespace capi;

namespace {

enum class InputMode { FILE, STRING };

enum class OutputFormat { JSON, SIMPLE, SCOREP, LEGACY_JSON};

struct Options {
  bool shouldWriteDOT{false};
  std::string dotFile;
  bool pathSensitive{false};
  bool replaceInlined{false};
  bool traverseVirtualDtors{false};
  std::string cgFile;
  std::string execFile;
  std::string queryStr;
  std::string outfile;
  bool debugMode{false};
  bool printSCCStats{false};
  InputMode mode{InputMode::FILE};
  OutputFormat outputFormat{OutputFormat::JSON};
};

void printHelp() {
  std::cout << "Usage: capi [options] <metacg_file>\n";
  std::cout << "Options:\n";
  std::cout << "-h   Print this help message.\n";
  std::cout
      << " -i <query>   Parse the selection query from the given string.\n";
  std::cout << " -f <file>      Use a selection query file.\n";
  std::cout << " -o <file>      The output IC file.\n";
  std::cout << " -v <verbosity>     Set verbosity level (0-3, default is 2). "
               "Passing -v without argument sets it to 3.\n";
  std::cout << " --write-dot <file>  Write a dotfile of the selected "
               "call-graph subset.\n";
  std::cout << " --replace-inlined <binary>  Replaces inlined functions with "
               "parents. Requires passing the executable.\n";
  std::cout << " --output-format <output_format>  Set the file format. Options "
               "are \"json\" (default), \"scorep\", \"legacy_json\" and \"simple\"\n";
  std::cout << " --debug  Enable debugging mode.\n";
  std::cout << " --print-scc-stats  Prints information about the strongly "
               "connected components (SCCs) of this call graph.\n";
  std::cout
      << " --traverse-virtual-dtors Enable traversal of virtual destructors, "
         "which may lead to an over-approximation of the function set.\n";
  std::cout << " --path-sensitive Specify call paths in the generated measurement configuration. Requires the input graph"
               "to be a forest.\n";
}


bool parseOptions(int argc, char** argv, Options& opts) {
  if (argc < 3) {
    std::cerr << "Missing input arguments.\n";
    printHelp();
    return false;
  }

  for (int i = 1; i < argc; i++) {
    std::string arg = argv[i];
    if (arg.length() > 1 && arg[0] == '-') {
      if (arg.length() > 2 && arg[1] == '-') {
        auto option = arg.substr(2);
        if (option == "write-dot") {
          opts.shouldWriteDOT = true;
          if (++i >= argc) {
            std::cerr << "Need to pass a name for the output dot file. \n";
            printHelp();
            return false;
          }
          opts.dotFile = argv[i];
        } else if (option == "debug") {
          opts.debugMode = true;
        } else if (option == "replace-inlined") {
          opts.replaceInlined = true;
          if (++i >= argc) {
            std::cerr << "Need to pass the target executable after "
                         "--replaced-inline. \n";
            printHelp();
            return false;
          }
          opts.execFile = argv[i];
        } else if (option == "output-format") {
          if (++i >= argc) {
            std::cerr << "Output format must be set to 'simple', 'json', 'legacy_json' or 'scorep'. \n";
            printHelp();
            return false;
          }
          std::string outputFormatStr = argv[i];
          if (outputFormatStr == "simple") {
            opts.outputFormat = OutputFormat::SIMPLE;
          } else if (outputFormatStr == "json") {
            opts.outputFormat = OutputFormat::JSON;
          } else if (outputFormatStr == "legacy_json") {
            opts.outputFormat = OutputFormat::LEGACY_JSON;
          } else if (outputFormatStr == "scorep") {
            opts.outputFormat = OutputFormat::SCOREP;
          } else {
            std::cerr << "Unsupported output format.\n";
            printHelp();
            return false;
          }
        } else if (option == "print-scc-stats") {
          opts.printSCCStats = true;
        } else if (option == "traverse-virtual-dtors") {
          opts.traverseVirtualDtors = true;
        } else if (option == "path-sensitive") {
          opts.pathSensitive = true;
        } else {
          std::cerr << "Invalid parameter --" << option << "\n";
          printHelp();
          return false;
        }
      } else {
        auto option = arg.substr(1);
        if (option == "f") {
          if (++i >= argc) {
            std::cerr << "Need to pass a selection spec file after -f\n";
            printHelp();
            return false;
          }
          opts.mode = InputMode::FILE;
          opts.queryStr = argv[i];
        } else if (option == "i") {
          if (++i >= argc) {
            std::cerr << "Need to pass a selection spec string after -i\n";
            printHelp();
            return EXIT_FAILURE;
          }
          opts.mode = InputMode::STRING;
          opts.queryStr = argv[i];
        } else if (option == "o") {
          if (++i >= argc) {
            std::cerr << "Need to pass an output file after -o\n";
            printHelp();
            return false;
          }
          opts.outfile = argv[i];
        } else if (option == "v") {
          int lvl = LOG_STATUS;
          if (i + 1 < argc) {
            lvl = std::stoi(argv[++i]);
            if (lvl < LOG_NONE || lvl > LOG_EXTRA) {
              std::cerr << "Verbosity must be an integer between 0 and 3\n";
              printHelp();
              return false;
            }
          }
          verbosity = static_cast<LogLevel>(lvl);
        } else if (option == "h") {
          printHelp();
        } else {
          std::cerr << "Unsupported option: " << option << "\n";
          printHelp();
          return false;
        }
      }
      continue;
    }
    if (opts.cgFile.empty()) {
      opts.cgFile = arg;
      continue;
    }
  }
  return true;
}

}



int main(int argc, char **argv) {
  std::cout << "CaPI Version " << CAPI_VERSION_MAJOR << "." << CAPI_VERSION_MINOR << "\n";
  std::cout << "Git revision: " << CAPI_GIT_SHA1 << "\n";

  Options opts;
  if (!parseOptions(argc, argv, opts)) {
    std::cerr << "Invalid input arguments - exiting...\n";
    return EXIT_FAILURE;
  }

  std::string queryStr = opts.queryStr;
  if (opts.mode == InputMode::FILE) {
    queryStr = loadFromFile(queryStr);
  }

  // Trying to parse the query here first so that we can abort before waiting for the call graph to load.
  auto testAst = parseSelectionQuery(queryStr);
  if (!testAst) {
    std::cerr << "Failed to parse selection query\n";
    return EXIT_FAILURE;
  }

  std::cout << "Loading call graph from " << opts.cgFile << "\n";

  metacg::io::FileSource fileSrc(opts.cgFile);
  auto reader = metacg::io::createReader(fileSrc);
  if (!reader) {
    std::cerr << "Unable to create reader for input file " << opts.cgFile << "\n";
    return EXIT_FAILURE;
  }

  auto cg = reader->read();
  if (!cg) {
    std::cerr << "Failed to read call graph\n";
    return EXIT_FAILURE;
  }

  // Attach demangled names as metadata
  demangleNames(*cg);

  std::cout << "Loaded CG with " << cg->size() << " nodes\n";

  // Create selection runner
  SelectionRunner runner(*cg, opts.traverseVirtualDtors);

  std::cout << "Running graph analysis...\n";

  // Running SCC analysis and printing stats
  SCCAnalysisResults sccResults;
  {
    Timer sccTimer("SCC Analysis took ", std::cout);
    sccResults = std::move(computeSCCs(runner.getTraversalHelper(), true));
  }
  if (opts.printSCCStats) {
    auto largestSCC =
        std::max_element(sccResults.sccs.begin(), sccResults.sccs.end(),
                         [](const auto &sccA, const auto &sccB) {
                           return sccA.size() < sccB.size();
                         });
    int numLargerOne = 0;
    int numLargerTwo = 0;
    for (const auto &scc : sccResults.sccs) {
      if (scc.size() > 1) {
        numLargerOne++;
        if (scc.size() > 2) {
          numLargerTwo++;
        }
      }
    }
    std::cout << "Number of SCCs: " << sccResults.size() << "\n";
    std::cout << "Largest SCC: " << largestSCC->size() << "\n";
    std::cout << "Number of SCCs containing more than 1 node: " << numLargerOne
              << "\n";
    std::cout << "Number of SCCs containing more than 2 nodes: " << numLargerTwo
              << "\n";
  }

  // Print AST after parsing
  runner.onASTParsed([&queryStr](SpecAST& ast) {
    std::cout << "AST for " << stripComments(queryStr) << ":\n";
    std::cout << "------------------\n";
    ast.dump(std::cout);
    std::cout << "\n";
    std::cout << "------------------\n";
    return true;
  });

  // Print AST after pre-processing
  runner.onASTPostProcessed([&queryStr](SpecAST& ast) {
    std::cout << "AST after pre-processing:\n";
    std::cout << "------------------\n";
    ast.dump(std::cout);
    std::cout << "\n";
    std::cout << "------------------\n";
    return true;
  });

  // Print pipeline before running
  runner.onSelectorGraphBuilt([](SelectorGraph& graph) {
    std::cout << "Selector pipeline:\n";
    std::cout << "------------------\n";
    dumpSelectorGraph(std::cout, graph);
    std::cout << "------------------\n";
    std::cout << "Running selector pipeline...\n";
    return true;
  });

  // Print result stats for each instrumentation action
  runner.onSelectionResult([](InstrumentationAction& action, FunctionSet& selection) {

      switch (action.type) {
        case capi::InstrumentationType::ALWAYS_INSTRUMENT: {
          std::cout << "Selected " << selection.size()
                    << " functions for instrumentation.\n";
          break;
        }
        case capi::InstrumentationType::BEGIN_TRIGGER: {
          std::cout << "Selected " << selection.size()
                    << " functions triggering the start of measurement.\n";
          break;
        }
        case capi::InstrumentationType::END_TRIGGER: {
          std::cout << "Selected " << selection.size()
                    << " functions triggering the end of measurement.\n";
          break;
        }
        case capi::InstrumentationType::SCOPE_TRIGGER: {
          std::cout << "Selected " << selection.size()
                    << " functions triggering scope measurement.\n";
          break;
        }
        default:
          assert(false && "Unhandled instrumentation type");
      }
    return true;
  });

  // Apply inline compensation, if requested
  if (opts.replaceInlined) {
    // Needs to be copied by-value, otherwise it will go out of scope!
    auto symSets = loadSymbolSets(opts.execFile);
    runner.onSelectionResult([symSets, &runner](InstrumentationAction& action, FunctionSet& selection){
      // Only run inline compensation for functions that are selected.
      if (action.type == InstrumentationType::ALWAYS_INSTRUMENT) {
        if (symSets.empty()) {
          std::cout << "Skipping inline compensation.\n";
        } else {
          selection =
              replaceInlinedFunctions(symSets, selection, runner.getTraversalHelper());
          std::cout << selection.size()
                    << " functions selected after inline compensation.\n";
        }
      }
      return true;
    });
  }

  // Also build legacy filter file, if needed
  FunctionFilter filter;
  bool legacyExport = opts.outputFormat != OutputFormat::JSON || opts.shouldWriteDOT;

  if (legacyExport) {
    runner.onSelectionResult([&filter](InstrumentationAction& action, FunctionSet& selection) {
      // Legacy function filter
      for (auto& f : selection) {
        filter.addIncludedFunction(f->getFunctionName(), action.type);
        assert(f->has<CaPIMD>());
        if (action.type == ALWAYS_INSTRUMENT && f->get<CaPIMD>()->value.isTrigger) {
          filter.addIncludedFunction(f->getFunctionName(), InstrumentationType::SCOPE_TRIGGER);
        }
      }
      return true;
    });
  }


  // Execute the query
  auto resultOrErr = runner.runQuery(queryStr, opts.pathSensitive, opts.debugMode);

  if (!resultOrErr) {
    std::cerr << "Selection query failed with error: " << resultOrErr.error() << "\n";
    return EXIT_FAILURE;
  }

  auto mc = *resultOrErr;

  std::string outfile = opts.outfile;
  if (outfile.empty()) {
    const char* fileEnding = opts.outputFormat == OutputFormat::JSON || opts.outputFormat == OutputFormat::LEGACY_JSON ? ".json" : ".filt";
    outfile = opts.cgFile.substr(0, opts.cgFile.find_last_of('.')) + fileEnding;
  }

  {
    bool writeSuccess{false};
    switch (opts.outputFormat) {
      case OutputFormat::SIMPLE:
        writeSuccess = writeSimpleFilterFile(filter, outfile);
        break;
      case OutputFormat::SCOREP:
        writeSuccess = writeScorePFilterFile(filter, outfile);
        break;
      case OutputFormat::JSON:
        writeSuccess = write(mc, outfile);
        break;
      case OutputFormat::LEGACY_JSON:
        writeSuccess = writeJSONFilterFile(filter, outfile);
        break;
    }
    if (!writeSuccess) {
      std::cerr << "Error: Writing result file failed.\n";
      return EXIT_FAILURE;
    }
  }

  if (opts.shouldWriteDOT) {
    std::ofstream os(opts.dotFile);
    if (os.is_open()) {
      writeDOT(runner.getTraversalHelper(), filter, {}, os);
    } else {
      std::cerr << "Could not write DOT file to '" << opts.dotFile << "'.\n";
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}