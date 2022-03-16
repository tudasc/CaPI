//
// Created by sebastian on 14.10.21.
//

#include "ReadMetaCG.h"

#include "ReadMetaCG.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

#include <string>

using namespace llvm;

namespace {
static llvm::RegisterPass<ReadMetaCG>
    passreg("readmetacg", "Read MetaCG database from file", false, false);
}

static cl::opt<std::string> ClCGFile("metacg-file",
                                     cl::desc("Location of the MetaCG file"),
                                     cl::Hidden, cl::init("cg.ipcg"));

static cl::opt<std::string>
    ClCfgFile("inst-config",
              cl::desc("Location of instrumentation config file"), cl::Hidden,
              cl::init("ic.json"));

// Used by LLVM pass manager to identify passes in memory
char ReadMetaCG::ID = 0;

ReadMetaCG::ReadMetaCG() : FunctionPass(ID) {}

void ReadMetaCG::getAnalysisUsage(llvm::AnalysisUsage &AU) const {
  AU.setPreservesCFG();
}

bool ReadMetaCG::doInitialization(llvm::Module &m) {

  outs() << "Running ReadMetaCG on module " << m.getName() << "\n";
  if (ClCGFile.empty()) {
    errs() << "Need to specify the location of the MetaCG file.\n";
    return false;
  }

  MetaCGReader reader(ClCGFile.getValue());
  if (!reader.read()) {
    return false;
  }

  this->functionInfo = reader.getFunctionInfo();

  return false;

  //    for (llvm::Function& f : m.functions()) {
  //        if (!f.hasName())
  //            errs() << "Function has no name!\n";
  //    }

  //    bool changed = llvm::count_if(m.functions(), [&](auto& f) { errs() <<
  //    f.getName(); return runOnFunc(f); }) > 0;
  //
  //    return changed;
}

bool markForInstrumentation(llvm::Function &f) {
  StringRef entryAttr = "instrument-function-entry";
  StringRef exitAttr = "instrument-function-exit";
  if (f.hasFnAttribute(entryAttr) || f.hasFnAttribute(exitAttr)) {
    llvm::errs() << "Function is already marked for instrumentation: "
                 << f.getName() << "\n";
    // TODO: Check if existing attribute is for GNU profiling interface
    return false;
  }
  StringRef entryFn = "__cyg_profile_func_enter";
  StringRef exitFn = "__cyg_profile_func_exit";

  f.addFnAttr(entryAttr, entryFn);
  f.addFnAttr(exitAttr, exitFn);
  return true;
}

bool ReadMetaCG::runOnFunction(llvm::Function &f) {
  llvm::errs() << "Running on function: " << f.getName() << "\n";
  auto it = functionInfo.find(f.getName());
  if (it == functionInfo.end()) {
    llvm::errs() << "Function not listed: " << f.getName() << "\n";
    return false;
  }
  auto info = it->second;
  if (info.instrument) {
    return markForInstrumentation(f);
  }
  return false;
}

bool ReadMetaCG::doFinalization(llvm::Module &) { return false; }

static void registerClangPass(const llvm::PassManagerBuilder &,
                              llvm::legacy::PassManagerBase &PM) {
  PM.add(new ReadMetaCG());
}
static RegisterStandardPasses
    RegisterClangPass(PassManagerBuilder::EP_EarlyAsPossible,
                      registerClangPass);
