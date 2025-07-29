//
// Created by sebastian on 28.10.21.
//

#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <unordered_set>

#include "../support/Timer.h"
#include "DOTWriter.h"
#include "Demangle.h"
#include "FunctionFilter.h"
#include "MeasurementConfig.h"
#include "MeasurementConfigIO.h"
#include "Preprocessor.h"
#include "SCC.h"
#include "SelectorBuilder.h"
#include "SelectorGraph.h"
#include "SpecParser.h"
#include "SymbolRetriever.h"
#include "TraversalHelper.h"
#include "capi_version.h"
#include "support/Logging.h"

#include "StatementCountAnalysis.h"
#include "io/MCGReader.h"
#include "metadata/BuiltinMD.h"
#include "metadata/CaPIMD.h"

using namespace capi;

CAPI_DEFINE_VERBOSITY

namespace capi {
bool traverseVirtualDtors{false};
}

namespace {

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
}

enum class InputMode { FILE, STRING };

enum class OutputFormat { JSON, SIMPLE, SCOREP, LEGACY_JSON};

ASTPtr parseSelectionSpec(std::string specStr) {
  auto stripped = stripComments(specStr);
  SpecParser parser(stripped);
  auto ast = parser.parse();
  return ast;
}

std::string loadFromFile(std::string_view filename) {
  if (filename.empty()) {
    std::cerr << "Need to specify either a preset or pass a selection file.\n";
    printHelp();
    return {};
  }

  std::ifstream in(std::string{filename});

  std::string specStr;

  std::string line;
  while (std::getline(in, line)) {
    specStr += line + '\n';
  }
  return specStr;
}

FunctionSet replaceInlinedFunctions(const SymbolSetList &symSets,
                                    const FunctionSet &functions,
                                    TraversalHelper &helper) {

  FunctionSet newSet;

  int numAdded = 0;

  std::function<void(const metacg::CgNode &, bool,
                     std::unordered_set<const metacg::CgNode *>)>
      addValidCallers =
          [&](const metacg::CgNode &node, bool trigger,
              std::unordered_set<const metacg::CgNode *> visited) {
            visited.insert(&node);
            auto nodeInfo = helper.get(&node);
            for (auto *caller : nodeInfo.getCallers()) {
              if (visited.find(caller) != visited.end()) {
                continue;
              }
              if (findSymbol(symSets, caller->getFunctionName())) {
                if (addToSet(newSet, caller)) {
                  if (trigger) {
                    assert(caller->has<CaPIMD>());
                    caller->get<CaPIMD>()->value.isTrigger = true;
                  }
                  numAdded++;
                }
              } else {
                addValidCallers(*caller, trigger, visited);
              }
            }
          };

  FunctionSet notFound;

  for (auto &fn : functions) {
    if (findSymbol(symSets, fn->getFunctionName())) {
      newSet.insert(fn);
    } else {
      notFound.insert(fn);
    }
  }
  std::cout << notFound.size()
            << " functions could not be located in the executable, likely due "
               "to inlining.\n";

  int numProcessed = 0;
  int numBetweenOutputs = notFound.size() / 10;
  int nextOutput = numBetweenOutputs;

  for (auto &fn : notFound) {
    if (!fn) {
      std::cerr << "Unable to find function in call graph - skipping.\n";
      continue;
    }
    // Recursively looks for the first available callers and adds them.
    assert(fn->has<CaPIMD>());
    addValidCallers(*fn, fn->get<CaPIMD>()->value.isTrigger, {});
    numProcessed++;

    // Status output
    if (numBetweenOutputs >= 10) {
      if (numProcessed >= nextOutput) {
        logInfo() << (int)((numProcessed / (float)notFound.size()) * 100)
                  << "% of inlined functions processed...\n";
        nextOutput += numBetweenOutputs;
      }
    }
  }

  std::cout << numAdded << " callers of missing functions added.\n";

  return newSet;
}
} // namespace

int main(int argc, char **argv) {

  std::cout << "CaPI Version " << CAPI_VERSION_MAJOR << "."
            << CAPI_VERSION_MINOR << "\n";
  bool verbose{false};

  std::cout << "Git revision: " << CAPI_GIT_SHA1 << "\n";

  if (argc < 3) {
    std::cerr << "Missing input arguments.\n";
    printHelp();
    return 1;
  }

  bool shouldWriteDOT{false};
  std::string dotFile;
  bool replaceInlined{false};
  std::string cgfile, specfile;
  std::string execFile;
  std::string specStr;
  std::string outfile;
  bool debugMode{false};
  bool printSCCStats{false};

  InputMode mode = InputMode::FILE;
  OutputFormat outputFormat = OutputFormat::JSON;

  for (int i = 1; i < argc; i++) {
    std::string arg = argv[i];
    if (arg.length() > 1 && arg[0] == '-') {
      if (arg.length() > 2 && arg[1] == '-') {
        auto option = arg.substr(2);
        if (option == "write-dot") {
          shouldWriteDOT = true;
          if (++i >= argc) {
            std::cerr << "Need to pass a name for the output dot file. \n";
            printHelp();
            return EXIT_FAILURE;
          }
          dotFile = argv[i];
        } else if (option == "debug") {
          debugMode = true;
        } else if (option == "replace-inlined") {
          replaceInlined = true;
          if (++i >= argc) {
            std::cerr << "Need to pass the target executable after "
                         "--replaced-inline. \n";
            printHelp();
            return EXIT_FAILURE;
          }
          execFile = argv[i];
        } else if (option == "output-format") {
          if (++i >= argc) {
            std::cerr
                << "Output format must be set to 'simple' or 'scorep'. \n";
            printHelp();
            return EXIT_FAILURE;
          }
          std::string outputFormatStr = argv[i];
          if (outputFormatStr == "simple") {
            outputFormat = OutputFormat::SIMPLE;
          } else if (outputFormatStr == "json") {
            outputFormat = OutputFormat::JSON;
          } else if (outputFormatStr == "legacy_json") {
            outputFormat = OutputFormat::LEGACY_JSON;
          } else if (outputFormatStr == "scorep") {
            outputFormat = OutputFormat::SCOREP;
          } else {
            std::cerr << "Unsupported output format.\n";
            printHelp();
            return EXIT_FAILURE;
          }
        } else if (option == "print-scc-stats") {
          printSCCStats = true;
        } else if (option == "traverse-virtual-dtors") {
          traverseVirtualDtors = true;
        } else {
          std::cerr << "Invalid parameter --" << option << "\n";
          printHelp();
          return EXIT_FAILURE;
        }
      } else {
        auto option = arg.substr(1);
        if (option == "f") {
          if (++i >= argc) {
            std::cerr << "Need to pass a selection spec file after -f\n";
            printHelp();
            return EXIT_FAILURE;
          }
          mode = InputMode::FILE;
          specfile = argv[i];
        } else if (option == "i") {
          if (++i >= argc) {
            std::cerr << "Need to pass a selection spec string after -i\n";
            printHelp();
            return EXIT_FAILURE;
          }
          mode = InputMode::STRING;
          specStr = argv[i];
        } else if (option == "o") {
          if (++i >= argc) {
            std::cerr << "Need to pass an output file after -o\n";
            printHelp();
            return EXIT_FAILURE;
          }
          outfile = argv[i];
        } else if (option == "v") {
          int lvl = LOG_STATUS;
          if (i + 1 < argc) {
            lvl = std::stoi(argv[++i]);
            if (lvl < LOG_NONE || lvl > LOG_EXTRA) {
              std::cerr << "Verbosity must be an integer between 0 and 3\n";
              printHelp();
              return EXIT_FAILURE;
            }
          }
          verbosity = static_cast<LogLevel>(lvl);
        } else if (option == "h") {
          printHelp();
        } else {
          std::cerr << "Unsupported option: " << option << "\n";
          printHelp();
          return EXIT_FAILURE;
        }
      }
      continue;
    }
    if (cgfile.empty()) {
      cgfile = arg;
      continue;
    }
  }

  if (mode == InputMode::FILE) {
    specStr = loadFromFile(specfile);
  }

  auto ast = parseSelectionSpec(specStr);

  if (!ast) {
    return EXIT_FAILURE;
  }

  std::cout << "AST for " << stripComments(specStr) << ":\n";
  std::cout << "------------------\n";
  ast->dump(std::cout);
  std::cout << "\n";
  std::cout << "------------------\n";

  InstrumentationHints hints;
  preprocessAST(*ast, hints);
  bool instHintsSpecified = !hints.empty();

  std::cout << "AST after pre-processing:\n";
  std::cout << "------------------\n";
  ast->dump(std::cout);
  std::cout << "\n";
  std::cout << "------------------\n";

  auto selectorGraph = buildSelectorGraph(*ast, !instHintsSpecified);

  if (!selectorGraph) {
    std::cerr << "Could not build selector pipeline.\n";
    return EXIT_FAILURE;
  }

  for (auto &hint : hints) {
    selectorGraph->addEntryNode(hint.selRefName);
  }

  std::cout << "Selector pipeline:\n";
  std::cout << "------------------\n";
  dumpSelectorGraph(std::cout, *selectorGraph);
  std::cout << "------------------\n";

  std::cout << "Loading call graph from " << cgfile << "\n";

  metacg::io::FileSource fileSrc(cgfile);
  auto reader = metacg::io::createReader(fileSrc);
  if (!reader) {
    std::cerr << "Unable to create reader for input file " << cgfile << "\n";
    return EXIT_FAILURE;
  }

  auto cg = reader->read();
  demangleNames(*cg);
  TraversalHelper helper(*cg, traverseVirtualDtors);

  std::cout << "Loaded CG with " << cg->size() << " nodes\n";

  std::cout << "Running graph analysis...\n";

  decltype(computeSCCs(helper, true)) sccResults;
  {
    Timer sccTimer("SCC Analysis took ", std::cout);
    sccResults = std::move(computeSCCs(helper, true));
  }
  if (printSCCStats) {
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

  // TODO: Add some kind of analysis management logic for selectors to request results
  StatementCountAnalysis sca;
  sca.run(helper);
  for (auto& [id, node] : cg->getNodes()) {
    long isc = node->get<ISCMD>()->value;
//    std::cout << "ISC for function " << node->getFunctionName() << ": " << isc << "\n";
  }

  std::cout << "Running selector pipeline...\n";

  auto result = runSelectorPipeline(*selectorGraph, helper, debugMode);

  // If hints empty, use full instrumentation of last defined selector instance
  if (hints.empty()) {
    hints.push_back({capi::InstrumentationType::ALWAYS_INSTRUMENT,
                     selectorGraph->getEntryNodes().back()->getName()});
  }

  bool legacyExport = outputFormat != OutputFormat::JSON;

  MeasurementConfig mc;

  // TODO: Get rid of old filter format
  FunctionFilter filter;

  for (auto &hint : hints) {
    auto it = result.find(hint.selRefName);
    if (it == result.end()) {
      logError() << "No selection results for '" << hint.selRefName << "'\n";
      continue;
    }
    auto selResult = it->second;

    switch (hint.type) {
    case capi::InstrumentationType::ALWAYS_INSTRUMENT: {
      std::cout << "Selected " << selResult.size()
                << " functions for instrumentation.\n";
      break;
    }
    case capi::InstrumentationType::BEGIN_TRIGGER: {
      std::cout << "Selected " << selResult.size()
                << " functions triggering the start of measurement.\n";
      break;
    }
    case capi::InstrumentationType::END_TRIGGER: {
      std::cout << "Selected " << selResult.size()
                << " functions triggering the end of measurement.\n";
      break;
    }
    case capi::InstrumentationType::SCOPE_TRIGGER: {
      std::cout << "Selected " << selResult.size()
                << " functions triggering scope measurement.\n";
      break;
    }
    default:
      assert(false && "Unhandled instrumentation type");
    }

    auto afterPostProcessing = selResult;

    // Only run inline compensation for functions that are actually measured.
    if (replaceInlined && hint.type == InstrumentationType::ALWAYS_INSTRUMENT) {
      auto symSets = loadSymbolSets(execFile);
      if (symSets.empty()) {
        std::cout << "Skipping inline compensation.\n";
      } else {
        afterPostProcessing =
            replaceInlinedFunctions(symSets, selResult, helper);
        std::cout << afterPostProcessing.size()
                  << " functions selected after inline compensation.\n";
      }
    }

    // Legacy function filter
    for (auto &f : afterPostProcessing) {
      filter.addIncludedFunction(f->getFunctionName(), hint.type);
      assert(f->has<CaPIMD>());
      if (hint.type == ALWAYS_INSTRUMENT && f->get<CaPIMD>()->value.isTrigger) {
        filter.addIncludedFunction(f->getFunctionName(),
                                   InstrumentationType::SCOPE_TRIGGER);
      }
    }

    // Measurement config
    for (auto &f : afterPostProcessing) {
      auto pathEntry = PathEntry{{}, hint.activeInvocations, "", {}}; // TODO: Measurement level?

      assert(f->has<CaPIMD>());
      if (hint.type == ALWAYS_INSTRUMENT && f->get<CaPIMD>()->value.isTrigger) {
        pathEntry.flags.push_back("scope_trigger");
      }
      mc.add(f->getFunctionName(), std::move(pathEntry));
    }
  }

  if (outfile.empty()) {
    const char* fileEnding = outputFormat == OutputFormat::JSON || outputFormat == OutputFormat::LEGACY_JSON ? ".json" : ".filt";
    outfile = cgfile.substr(0, cgfile.find_last_of('.')) + fileEnding;
  }

  {

    bool writeSuccess{false};
    switch (outputFormat) {
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
    }
  }

  if (shouldWriteDOT) {
    std::ofstream os(dotFile);
    if (os.is_open()) {
      writeDOT(helper, filter, {}, os);
    } else {
      logError() << "Could not write DOT file to '" << dotFile << "'.\n";
    }
  }

  return EXIT_SUCCESS;
}