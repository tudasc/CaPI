//
// Created by sebastian on 10.11.21.
//

#include "CallGraph.h"
#include "DOTWriter.h"
#include "MetaCGReader.h"
#include "Selectors.h"

#include <fstream>
#include <iostream>

// erase_if for maps is only available in C++20
template <typename ContainerT, typename PredicateT>
void erase_if(ContainerT &items, const PredicateT &predicate) {
  for (auto it = items.begin(); it != items.end();) {
    if (predicate(*it))
      it = items.erase(it);
    else
      ++it;
  }
}

enum class Tool { CHECK_CONNECTED, WRITE_DOT };

std::vector<std::string> getSubtreeFunctions(CallGraph &cg,
                                             const std::string &targetFn) {
  SelectorRunner runner(cg);
  auto selector =
      selector::onCallPathFrom(selector::byName(targetFn, selector::all()));
  return runner.run(*selector);
}

int main(int argc, char **argv) {
  if (argc < 4) {
    std::cerr << "Missing input file argument\n";
    return 1;
  }

  std::string toolName = argv[1];
  std::string infile = argv[2];

  Tool tool;

  if (toolName == "check-connected") {
    tool = Tool::CHECK_CONNECTED;
    if (argc < 5) {
      std::cerr << "Missing second CG\n";
      return 1;
    }
  } else if (toolName == "write-dot") {
    tool = Tool::WRITE_DOT;
  } else {
    std::cerr << "Unknown utility tool. Available tools:\n";
    std::cerr << "check-connected cg_a function cg_b\n";
    std::cerr << "write-dot cg function\n";
    return 1;
  }

  std::string targetFn = argv[3];

  std::cout << "Loading CG from " << infile << "\n";

  MetaCGReader reader(infile);
  if (!reader.read()) {
    std::cerr << "Unable to load call graph\n";
    return 1;
  }

  auto functionInfo = reader.getFunctionInfo();

  auto cg = createCG(functionInfo);

  std::cout << "Loaded CG with " << cg->size() << " nodes\n";

  if (tool == Tool::WRITE_DOT) {

    auto targetNode = cg->get(targetFn);

    if (!targetNode) {
      std::cerr << "Function " << targetFn << " could not be found.\n";
      return 1;
    }

    auto result = getSubtreeFunctions(*cg, targetFn);
    std::cout << "Selected " << result.size() << " functions.\n";

    auto filteredFunctionInfo = functionInfo;
    erase_if(
        filteredFunctionInfo, [&result](const auto &entry) -> auto {
          return std::find(result.begin(), result.end(), entry.first) ==
                 result.end();
        });

    auto filteredCG = createCG(filteredFunctionInfo);

    std::cout << "Functions on CG subtree: " << filteredCG->size() << "\n";

    std::string dotfile = "cg_" + targetFn + ".dot";
    {
      std::ofstream os(dotfile);
      if (os.is_open()) {
        writeDOT(*filteredCG, os);
      }
    }
  } else if (tool == Tool::CHECK_CONNECTED) {
    std::string otherCGFile = argv[4];

    std::cout << "Loading CG from " << otherCGFile << "\n";

    MetaCGReader reader(otherCGFile);
    if (!reader.read()) {
      std::cerr << "Unable to load call graph\n";
      return 1;
    }
    auto otherFunctionInfo = reader.getFunctionInfo();
    auto otherCG = createCG(otherFunctionInfo);

    std::cout << "Loaded CG with " << otherCG->size() << " nodes\n";

    if (!otherCG->get(targetFn)) {
      std::cerr << "Target function does not exist\n";
      return 1;
    }
    // Determine functions in subtree starting at targetFn
    auto subtreeFns = getSubtreeFunctions(*otherCG, targetFn);

    std::cout << "Functions called from " << targetFn << ": "
              << subtreeFns.size() << "\n";

    // Run MPI filtering on first CG
    SelectorRunner runner(*cg);
    auto mpiSelector =
        selector::onCallPathTo(selector::byName("MPI_.*", selector::all()));
    auto selector = selector::byIncludeList(subtreeFns, std::move(mpiSelector));
    auto result = runner.run(*selector);

    std::cout << "Found " << result.size()
              << " selected functions that are called from " << targetFn
              << ".\n";
  }

  return 0;
}