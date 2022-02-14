//
// Created by sebastian on 14.10.21.
//

#include "ApplyInstrumentationFilter.h"

#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

#include <string>
#include <fstream>

using namespace llvm;

namespace {
    static llvm::RegisterPass<ApplyInstrumentationFilter> passreg("instfilter", "Read and apply instrumentation filter", false, false);
}

static cl::opt<std::string> ClFilterFile("instrument-filter", cl::desc("Location of instrumentation filter file"), cl::Hidden, cl::init("inst.filt"));

// Used by LLVM pass manager to identify passes in memory
char ApplyInstrumentationFilter::ID = 0;

ApplyInstrumentationFilter::ApplyInstrumentationFilter() : FunctionPass(ID) {
}

void ApplyInstrumentationFilter::getAnalysisUsage(llvm::AnalysisUsage &AU) const {
    AU.setPreservesCFG();
}

bool ApplyInstrumentationFilter::doInitialization(llvm::Module& m) {

    if (ClFilterFile.empty()) {
        errs() << "Need to specify the location of the filter file.\n";
        return false;
    }

    std::string infile = ClFilterFile.getValue();


    {
        std::ifstream in(infile);
        if (!in.is_open()) {
            auto filterEnv = std::getenv("INST_FILTER_FILE");
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
        while(std::getline(in, line)) {
            filterList.push_back(line);
        }
    }

    return false;
}

bool markForInstrumentation(llvm::Function& f) {
    llvm::errs() << "Instrumenting function: " << f.getName() << "\n";
    StringRef entryAttr = "instrument-function-entry-inlined";
    StringRef exitAttr = "instrument-function-exit-inlined";
    if (f.hasFnAttribute(entryAttr) || f.hasFnAttribute(exitAttr)) {
        llvm::errs() << "Function is already marked for instrumentation: " << f.getName() << "\n";
        // TODO: Check if existing attribute is for GNU profiling interface
        return false;
    }
    StringRef entryFn = "__cyg_profile_func_enter";
    StringRef exitFn = "__cyg_profile_func_exit";

    f.addFnAttr(entryAttr, entryFn);
    f.addFnAttr(exitAttr, exitFn);
    return true;
}

bool ApplyInstrumentationFilter::runOnFunction(llvm::Function& f) {
    llvm::errs() << "Running on function: " << f.getName() << "\n";
    auto it = std::find(filterList.begin(), filterList.end(), f.getName());
    if (it == filterList.end()) {
        return false;
    }
    return markForInstrumentation(f);
}

bool ApplyInstrumentationFilter::doFinalization(llvm::Module &) {
    return false;
}

static void registerClangPass(const llvm::PassManagerBuilder&, llvm::legacy::PassManagerBase& PM) {
    PM.add(new ApplyInstrumentationFilter());
}
static RegisterStandardPasses RegisterClangPass(PassManagerBuilder::EP_EarlyAsPossible, registerClangPass);
