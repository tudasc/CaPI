//
// Created by sebastian on 28.10.21.
//

#include "../Utils.h"
#include "SymbolRetriever.h"

CAPI_DEFINE_VERBOSITY

#ifdef CAPI_SCOREP_SUPPORT
#include "ScorePInit.h"
#endif

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <sstream>
#include <unistd.h>

#include "xray/xray_interface.h"

#define XRAY_NEVER_INSTRUMENT __attribute__((xray_never_instrument))


extern "C" {
void __cyg_profile_func_enter(void* fn, void* callsite) XRAY_NEVER_INSTRUMENT;
void __cyg_profile_func_exit(void* fn, void* callsite) XRAY_NEVER_INSTRUMENT;

};

#include <fstream>
#include <iostream>
#include <map>
#include <string>

#include "CallGraph.h"
#include "DOTWriter.h"
#include "FunctionFilter.h"
#include "MetaCGReader.h"
#include "SpecParser.h"
#include "SelectorGraph.h"
#include "SelectorBuilder.h"

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
    return R"(onCallPathTo(by_name("MPI_.*", %%)))";
//    return selector::onCallPathTo(selector::by_name("MPI_.*", selector::all()));
  case SelectionPreset::OPENFOAM_MPI:
    return R"(mpi=onCallPathTo(by_name("MPI_.*", %%)) exclude=join(byPath(".*\\/OpenFOAM\\/db\\/.*", %%), inlineSpecified(%%)) subtract(%mpi, %exclude))";
//    return selector::subtract(
//        selector::onCallPathTo(selector::by_name("MPI_.*", selector::all())),
//        selector::join(
//            selector::byPath(".*\\/OpenFOAM\\/db\\/.*", selector::all()),
//            selector::inlineSpecified(selector::all())));
  default:
    assert(false && "Preset not implemented");
  }
  return "";
}

}

using XRayHandlerFn = void (*)(int32_t, XRayEntryType);

void handleXRayEvent(int32_t id, XRayEntryType type) XRAY_NEVER_INSTRUMENT {
  uintptr_t addr = __xray_function_address(id);
  if (!addr) {
    return;
  }

  switch(type) {
  case XRayEntryType::ENTRY:
    __cyg_profile_func_enter(reinterpret_cast<void*>(addr), nullptr);
    break;
  case XRayEntryType::TAIL:
  case XRayEntryType::EXIT:
    __cyg_profile_func_exit(reinterpret_cast<void*>(addr), nullptr);
    break;
  default:
    logError() << "Unhandled XRay event type.\n";
    break;
  }

}

static void* cg_data;


extern "C" {
[[maybe_unused]] void initCaPI() XRAY_NEVER_INSTRUMENT {
  auto cg_json = std::string((char *)cg_data);

  logInfo() << "Initializing DynCaPI...\n";

  Timer timer("[Info] Initialization took ", std::cout);

  MetaCGReader reader(cg_json); // FIXME: Interface changed, expects file now
  if (!reader.read()) {
    std::cerr << "Unable to load call graph!\n";
    return;
  }

  // FIXME: This is all crappy legacy code. Needs to be updated to incorporate the changes from CaPI.cpp

  auto functionInfo = reader.getFunctionInfo();

  auto cg = createCG(functionInfo);

  auto specStr = getPreset(SelectionPreset::OPENFOAM_MPI);
  specStr = "all=onCallPathTo(%%)";
  auto specStrEnv = std::getenv("CAPI_SPEC");
  if (!specStrEnv) {
    logError() << "No spec specified!\n";
  } else {
    specStr = specStrEnv;
  }

  auto ast = parseSelectionSpec(specStr);

  if (!ast) {
    return;
  }

  std::cout << "AST for " << specStr << ":\n";
  std::cout << "------------------\n";
  ast->dump(std::cout);
  std::cout << "\n";
  std::cout << "------------------\n";

  auto selectorGraph = buildSelectorGraph(*ast, true);

  if (!selectorGraph) {
    std::cerr << "Could not build selector pipeline.\n";
    return;
  }

  std::cout << "Selector pipeline:\n";
  std::cout << "------------------\n";
  dumpSelectorGraph(std::cout, *selectorGraph);
  std::cout << "------------------\n";

  std::cout << "Loaded CG with " << cg->size() << " nodes\n";

  auto result = runSelectorPipeline(*selectorGraph, *cg, false);

  std::cout << "Selected " << result.size() << " functions.\n";

  // FIXME: Currently broken, need to adopt behavior of CaPI RT
  FunctionFilter filter;
  for (auto &f : result) {
//    filter.addIncludedFunction(f->getName());
  }

  auto execPath = getExecPath();
  auto execFilename = execPath.substr(execPath.find_last_of('/') + 1);


  auto symTables = loadMappedSymTables(execPath);

  size_t numFound = 0;
  size_t numInserted = 0;
  size_t numPatched = 0;

  SymbolTable globalTable;

  for (auto &&[startAddr, table] : symTables) {
    for (auto &&[addr, symName] : table.table) {
      auto addrInProc = mapAddrToProc(addr, table);
      if (filter.accepts(symName)) {
        globalTable[addrInProc] = symName;
        ++numInserted;
      }
      ++numFound;
    }
  }

  logInfo() << "Done retrieving symbols.\n";


  __xray_init();
  __xray_set_handler(handleXRayEvent);

  size_t maxFID = __xray_max_function_id();
  if (maxFID == 0) {
    logError() << "No functions instrumented.\n";
    return;
  }

  logInfo() << "Patching " << maxFID << " functions\n";

  for (int fid = 1; fid <= maxFID; ++fid) {
    uintptr_t addr = __xray_function_address(fid);
    if (!addr) {
      logError() << "Unable to determine address for function " << fid << "\n";
      continue;
    }
    if (auto it = globalTable.find(addr); it != globalTable.end()) {
      auto patchStatus = __xray_patch_function(fid);
      if (patchStatus == SUCCESS) {
        logInfo() << "Patched function: id=" << fid << ", name=" << it->second
                  << "\n";
        numPatched++;
      } else {
        logError() << "XRay patching failed: id=" << fid
                   << ", name=" << it->second << "\n";
      }
    } else {
      logError() << "Unable find symbol for function: id=" << fid
                 << ", addr=" << std::hex << addr << std::dec << "\n";
    }
  }

  logInfo() << "Functions found: " << numFound << "\n";
  logInfo() << "Functions registered: " << numInserted << "\n";
  logInfo() << "Functions patched: " << numPatched << "\n";

#ifdef CAPI_SCOREP_SUPPORT
  initScoreP(globalTable);
#endif
}


void getGCC(void* data) {
  cg_data = data;

}
}

