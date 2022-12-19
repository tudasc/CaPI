//
// Created by sebastian on 28.10.21.
//

#include <fstream>
#include <iostream>
#include <map>
#include <string>

#include "CallGraph.h"
#include "DOTWriter.h"
#include "FunctionFilter.h"
#include "MetaCGReader.h"
#include "Preprocessor.h"
#include "SelectorBuilder.h"
#include "SelectorGraph.h"
#include "SpecParser.h"

using namespace capi;

namespace {

void printHelp() {
  std::cout << "Usage: capi [options] cg_file\n";
  std::cout << "Options:\n";
  std::cout
      << " -p <preset>    Use a selection preset, where <preset> is one of "
         "'MPI','FOAM'.}\n";
  std::cout << " -f <file>      Use a selection spec file.\n";
  std::cout
      << " -i <specstr>   Parse the selection spec from the given string.\n";
  std::cout
      << " --write-dot  Write a dotfile of the selected call-graph subset.\n";
}

enum class InputMode { PRESET, FILE, STRING };

enum class SelectionPreset {
  MPI,
  OPENFOAM_MPI,
};

namespace {
std::map<std::string, SelectionPreset> presetNames = {
    {"MPI", SelectionPreset::MPI}, {"FOAM", SelectionPreset::OPENFOAM_MPI}};
}


ASTPtr parseSelectionSpec(std::string specStr) {
  SpecParser parser(specStr);
  auto ast = parser.parse();
  return ast;
}


std::string loadFromFile(std::string_view filename) {
  if (filename.empty()) {
    std::cerr << "Need to specify either a preset or pass a selection file.\n";
    printHelp();
    return nullptr;
  }

  std::ifstream in(std::string{filename});

  std::string specStr;

  std::string line;
  while (std::getline(in, line)) {
    specStr += line;
  }
  return specStr;
}

std::string getPreset(SelectionPreset preset) {
  switch (preset) {
  case SelectionPreset::MPI:
    return R"(onCallPathTo(byName("MPI_.*", %%)))";
    //    return selector::onCallPathTo(selector::byName("MPI_.*", selector::all()));
  case SelectionPreset::OPENFOAM_MPI:
    return R"(mpi=onCallPathTo(byName("MPI_.*", %%)) exclude=join(byPath(".*\\/OpenFOAM\\/db\\/.*", %%), inlineSpecified(%%)) subtract(%mpi, %exclude))";
    //    return selector::subtract(
    //        selector::onCallPathTo(selector::byName("MPI_.*", selector::all())),
    //        selector::join(
    //            selector::byPath(".*\\/OpenFOAM\\/db\\/.*", selector::all()),
    //            selector::inlineSpecified(selector::all())));
  default:
    assert(false && "Preset not implemented");
  }
  return "";
}

}

int main(int argc, char **argv) {

  if (argc < 3) {
    std::cerr << "Missing input arguments.\n";
    printHelp();
    return 1;
  }

  bool shouldWriteDOT{false};
  std::string cgfile, specfile;
  std::string specStr;
  SelectionPreset preset;

  InputMode mode = InputMode::FILE;

  for (int i = 1; i < argc; i++) {
    std::string arg = argv[i];
    if (arg.length() > 1 && arg[0] == '-') {
      if (arg.length() > 2 && arg[1] == '-') {
        auto option = arg.substr(2);
        if (option.compare("write-dot") == 0) {
          shouldWriteDOT = true;
        }
      } else {
        auto option = arg.substr(1);
        if (option.compare("p") == 0) {
          if (++i >= argc) {
            std::cerr << "Need to pass a selection preset after -p\n";
            printHelp();
            return EXIT_FAILURE;
          }
          auto presetStr = argv[i];
          if (auto it = presetNames.find(presetStr); it != presetNames.end()) {
            mode = InputMode::PRESET;
            preset = it->second;
          } else {
            std::cerr << "Invalid preset '" << presetStr << "'\n";
            printHelp();
            return EXIT_FAILURE;
          }

        } else if (option.compare("f") == 0) {
          if (++i >= argc) {
            std::cerr << "Need to pass a selection spec file after -f\n";
            printHelp();
            return EXIT_FAILURE;
          }
          mode = InputMode::FILE;
          specfile = argv[i];
        } else if (option.compare("i") == 0) {
          if (++i >= argc) {
            std::cerr << "Need to pass a selection spec string after -i\n";
            printHelp();
            return EXIT_FAILURE;
          }
          mode = InputMode::STRING;
          specStr = argv[i];
        }
      }
      continue;
    }
    if (cgfile.empty()) {
      cgfile = arg;
      continue;
    }
  }


  switch (mode) {
  case InputMode::FILE:
    specStr = loadFromFile(specfile);
    break;
  case InputMode::PRESET:
    specStr = getPreset(preset);
    break;
  case InputMode::STRING: {
    break;
  }
  default:
    break;
  }

  auto ast = parseSelectionSpec(specStr);

  if (!ast) {
    return EXIT_FAILURE;
  }

  std::cout << "AST for " << specStr << ":\n";
  std::cout << "------------------\n";
  ast->dump(std::cout);
  std::cout << "\n";
  std::cout << "------------------\n";

  preprocessAST(*ast);

  std::cout << "AST after pre-processing:\n";
  std::cout << "------------------\n";
  ast->dump(std::cout);
  std::cout << "\n";
  std::cout << "------------------\n";

  auto selectorGraph = buildSelectorGraph(*ast);

  if (!selectorGraph) {
    std::cerr << "Could not build selector pipeline.\n";
    return EXIT_FAILURE;
  }

  std::cout << "Selector pipeline:\n";
  std::cout << "------------------\n";
  dumpSelectorGraph(std::cout, *selectorGraph);
  std::cout << "------------------\n";

  std::cout << "Loading call graph from " << cgfile << "\n";


  MetaCGReader reader(cgfile);
  if (!reader.read()) {
    std::cerr << "Unable to load call graph!\n";
    return EXIT_FAILURE;
  }

  auto functionInfo = reader.getFunctionInfo();

  auto cg = createCG(functionInfo);

  std::cout << "Loaded CG with " << cg->size() << " nodes\n";

  auto result = runSelectorPipeline(*selectorGraph, *cg);

  std::cout << "Selected " << result.size() << " functions.\n";

  auto outfile = cgfile.substr(0, cgfile.find_last_of('.')) + ".filt";

  {
    FunctionFilter filter;
    for (auto &f : result) {
      filter.addIncludedFunction(f);
    }
    if (!writeScorePFilterFile(filter, outfile)) {
      std::cerr << "Error: Writing filter file failed.\n";
    }
  }

  if (shouldWriteDOT) {
    std::ofstream os("cg.dot");
    if (os.is_open()) {
      writeDOT(*cg, os);
    }
  }

  return EXIT_SUCCESS;
}
