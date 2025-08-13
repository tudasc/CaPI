//
// Created by sebastian on 12.08.25.
//

#include "NeSmiKPass.h"

#include "llvm/IR/Instructions.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Support/CommandLine.h"
//#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Passes/PassBuilder.h"

#include <fstream>
#include <string>

using namespace llvm;

// New PM registration
extern "C" LLVM_ATTRIBUTE_WEAK PassPluginLibraryInfo llvmGetPassPluginInfo() {
  return {
      LLVM_PLUGIN_API_VERSION, "nesmik-inst", "v1.0", [](llvm::PassBuilder& PB) {
        PB.registerOptimizerLastEPCallback([&](ModulePassManager &MPM, OptimizationLevel O, ThinOrFullLTOPhase) {
          MPM.addPass(createModuleToFunctionPassAdaptor(NeSmiKPass()));
          return true;
        });
      }
  };
}

static bool isMPIInit(StringRef Name) {
  return Name == "MPI_Init" || Name == "MPI_Init_thread";
}

static bool isMPIFinalize(StringRef Name) {
  return Name == "MPI_Finalize";
}

static void instrument(Instruction* I, FunctionCallee& Callee) {
  IRBuilder<> IRB(I);
  IRB.CreateCall(Callee);
}

llvm::PreservedAnalyses NeSmiKPass::run(llvm::Function &F, llvm::FunctionAnalysisManager &) {

  Module &M = *F.getParent();
  LLVMContext &C = F.getContext();

  FunctionCallee InitFn = M.getOrInsertFunction("dyncapi_nesmik_init", Type::getVoidTy(C));
  FunctionCallee FinalizeFn = M.getOrInsertFunction("dyncapi_nesmik_finalize", Type::getVoidTy(C));

  bool DidInstrument = false;

  for (auto& BB : F) {
    for (auto& I : BB) {
      if (auto CI = dyn_cast<CallBase>(&I); CI) {
        auto Callee = CI->getCalledFunction();
        if (!Callee || CI->isIndirectCall()) {
           continue;
        }
        auto Name = Callee->getName();
        if (isMPIInit(Name)) {
          assert(I.getNextNode() && "Insertion point is null");
          instrument(I.getNextNode(), InitFn);
          DidInstrument = true;
          llvm::outs() << "Instrumented " << Name << " call in " << F.getName() << "\n";
        } else if (isMPIFinalize(Name)) {
          instrument(&I, FinalizeFn);
          DidInstrument = true;
          llvm::outs() << "Instrumented " << Name << " call in " << F.getName() << "\n";
        }
      }
    }
  }

  if (DidInstrument)
    return PreservedAnalyses::all();

  return PreservedAnalyses::none();
}
