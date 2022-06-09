//
// Created by sebastian on 14.10.21.
//

#include "CaPIInstrumenter.h"

#include "llvm/IR/Instructions.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

#include <fstream>
#include <string>

using namespace llvm;

namespace {
static llvm::RegisterPass<CaPIInstrumenter>
    passreg("capi-inst", "Read and apply CaPI instrumentation configuration", false,
            false);
}

static cl::opt<std::string>
    ClFilterFile("inst-filter",
                 cl::desc("Location of instrumentation filter file"),
                 cl::Hidden, cl::init("inst.filt"));

static cl::opt<std::string>
    ClInstAPI("inst-api",
                 cl::desc("Instrumentation API. Select \"gnu\" or \"talp\""),
                 cl::Hidden, cl::init("gnu"));

// Used by LLVM pass manager to identify passes in memory
char CaPIInstrumenter::ID = 0;

CaPIInstrumenter::CaPIInstrumenter() : FunctionPass(ID) {}

void CaPIInstrumenter::getAnalysisUsage(
    llvm::AnalysisUsage &AU) const {
  AU.setPreservesCFG();
}

bool CaPIInstrumenter::doInitialization(llvm::Module &m) {

  if (ClInstAPI == "talp") {
    api = InstAPI::TALP;
  } else if (ClInstAPI == "gnu") {
    api = InstAPI::GNU;
  } else {
    llvm::errs() << "Invalid instrumentation API: " << ClInstAPI << ". Defaulting to GNU interface.\n";
    api = InstAPI::GNU;
  }

  auto filterEnv = std::getenv("CAPI_FILTER_FILE");
  if (ClFilterFile.empty() && !filterEnv) {
    errs() << "Need to specify the location of the filter file.\n";
    return false;
  }

  std::string infile = ClFilterFile.getValue();
  {
    std::ifstream in(infile);
    if (!in.is_open()) {
      if (filterEnv) {
        in.open(filterEnv);
        if (!in.is_open()) {
          llvm::errs() << "Error: Opening file failed: " << filterEnv << "\n";
          return false;
        }
      } else {
        llvm::errs() << "Error: Opening file failed: " << infile << "\n";
        return false;
      }
    }

    std::string line;
    while (std::getline(in, line)) {
      filterList.push_back(line);
    }
  }

  return false;
}

bool markForGNUInstrumentation(llvm::Function &f) {
  llvm::errs() << "Instrumenting function: " << f.getName() << "\n";
  StringRef entryAttr = "instrument-function-entry-inlined";
  StringRef exitAttr = "instrument-function-exit-inlined";
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



bool markForTALPInstrumentation(llvm::Function &f) {
  llvm::errs() << "Instrumenting function: " << f.getName() << "\n";
  StringRef talpAttr = "instrument-talp-region";
  if (f.hasFnAttribute(talpAttr) ) {
    llvm::errs() << "Function is already marked for TALP region instrumentation: "
                 << f.getName() << "\n";
    return false;
  }

  f.addFnAttr(talpAttr, "true");
  return true;
}

bool CaPIInstrumenter::runOnFunction(llvm::Function &f) {
  llvm::errs() << "Running on function: " << f.getName() << "\n";
  auto it = std::find(filterList.begin(), filterList.end(), f.getName());
  if (it == filterList.end()) {
    return false;
  }
  switch(api) {
  case InstAPI::GNU:
    return markForGNUInstrumentation(f);
  case InstAPI::TALP:
    return markForTALPInstrumentation(f);
  }
}

bool CaPIInstrumenter::doFinalization(llvm::Module &) {
  return false;
}

static void registerClangPass(const llvm::PassManagerBuilder &,
                              llvm::legacy::PassManagerBase &PM) {
  PM.add(new CaPIInstrumenter());
}
static RegisterStandardPasses
    RegisterClangPass(PassManagerBuilder::EP_EarlyAsPossible,
                      registerClangPass);
