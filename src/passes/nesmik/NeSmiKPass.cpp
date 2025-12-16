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

// TODO: Include header?
//void __xray_customevent(const void *data, size_t size);

static cl::opt<bool>
    ClEmitXRay("emit-xray-events",
              cl::desc("Emit custom XRay events instead of using static instrumentation."),
              cl::Hidden, cl::init(false));

enum class EventType {
  INIT, FINALIZE, PAR_REGION_ENTER, PAR_REGION_EXIT
};

// New PM registration
extern "C" LLVM_ATTRIBUTE_WEAK PassPluginLibraryInfo llvmGetPassPluginInfo() {
  return {
      LLVM_PLUGIN_API_VERSION, "nesmik-inst", "v1.0", [](llvm::PassBuilder& PB) {
        PB.registerOptimizerLastEPCallback([&](ModulePassManager &MPM, OptimizationLevel O, ThinOrFullLTOPhase) {
          MPM.addPass(NeSmiKPass());
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

static bool isOMPFork(StringRef Name) {
  return Name == "__kmpc_fork_call"; // TODO: Other calls to instrument?
}

llvm::PreservedAnalyses NeSmiKPass::run(llvm::Module &M, llvm::ModuleAnalysisManager &) {

  if (ClEmitXRay) {
    IRBuilder<> IRB(M.getContext());
    this->InitEventStr = IRB.CreateGlobalString("dyncapi_init", "init.str", 0, &M);
    this->ExitEventStr = IRB.CreateGlobalString("dyncapi_finalize", "finalize.str", 0, &M);
    this->ParRegionEnterEventStr = IRB.CreateGlobalString("dyncapi_par_region_enter", "par_enter.str", 0, &M);
    this->ParRegionExitEventStr = IRB.CreateGlobalString("dyncapi_par_region_exit", "par_exit.str", 0, &M);
    llvm::outs() << "Instrumenting with XRay custom events\n";
  }

  bool Modified = false;
  for (auto& F: M.functions()) {
    Modified |= runOnFunction(F);
  }

  if (Modified)
    return PreservedAnalyses::all();

  return PreservedAnalyses::none();

}

bool NeSmiKPass::runOnFunction(llvm::Function& F) {

  Module &M = *F.getParent();
  LLVMContext &C = F.getContext();

  FunctionCallee InitFn = M.getOrInsertFunction("dyncapi_nesmik_init", Type::getVoidTy(C));
  FunctionCallee FinalizeFn = M.getOrInsertFunction("dyncapi_nesmik_finalize", Type::getVoidTy(C));
  FunctionCallee ParEnterFn = M.getOrInsertFunction("dyncapi_par_region_enter", Type::getVoidTy(C));
  FunctionCallee ParExitFn = M.getOrInsertFunction("dyncapi_par_region_exit", Type::getVoidTy(C));

//  llvm::Type *CharTy = llvm::Type::getInt8Ty(C);
//  llvm::PointerType *CharPtrTy = llvm::PointerType::getUnqual(CharTy);

  FunctionCallee XRayCustomEvent = M.getOrInsertFunction("llvm.xray.customevent", Type::getVoidTy(C), PointerType::get(C,0), Type::getInt64Ty(C));

  bool EmitXRayEvents = ClEmitXRay;

  auto instrument = [&](Instruction* I, EventType type) {

    IRBuilder<> IRB(I);
    if (EmitXRayEvents) {
      Value* EventStr;

      switch (type) {
        case EventType::INIT:
          EventStr = InitEventStr;
          break;
        case EventType::FINALIZE:
          EventStr = ExitEventStr;
          break;
        case EventType::PAR_REGION_ENTER:
          EventStr = ParRegionEnterEventStr;
          break;
        case EventType::PAR_REGION_EXIT:
          EventStr = ParRegionExitEventStr;
          break;
        default:
          llvm_unreachable("Unhandled event type");
      }
      assert(EventStr);

      auto *GV = llvm::cast<llvm::GlobalVariable>(
          EventStr);

      auto *CDA = llvm::cast<llvm::ConstantDataArray>(GV->getInitializer());
      unsigned LengthWithNull = CDA->getType()->getArrayNumElements();
      auto* LenConst =
          llvm::ConstantInt::get(llvm::Type::getInt64Ty(GV->getContext()),
                                 LengthWithNull);

      IRB.CreateCall(XRayCustomEvent, {EventStr, LenConst});
    } else {
      FunctionCallee* Callee = nullptr;
      switch(type) {
        case EventType::INIT:
          Callee = &InitFn;
          break;
        case EventType::FINALIZE:
          Callee = &FinalizeFn;
          break;
        case EventType::PAR_REGION_ENTER:
          Callee = &ParEnterFn;
          break;
        case EventType::PAR_REGION_EXIT:
          Callee = &ParExitFn;
          break;
        default:
          llvm_unreachable("Unhandled event type");
      }
      assert(Callee && "Callee must not be null");
      IRB.CreateCall(*Callee);
    }
  };

  auto getFirstPostCallInsertPt = [](auto& CI) {
    Instruction* InsertPt = CI->getNextNode();
    if (!InsertPt) {
      if (auto II = dyn_cast<InvokeInst>(CI); II) {
        InsertPt = &(*II->getNormalDest()->getFirstInsertionPt());
      }
      assert(InsertPt && "Could not find insertion point after call");
    }
    return InsertPt;
  };

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
          Instruction* InsertPt = getFirstPostCallInsertPt(CI);
          instrument(InsertPt, EventType::INIT);
          DidInstrument = true;
          llvm::outs() << "Instrumented " << Name << " call in " << F.getName() << "\n";
        } else if (isMPIFinalize(Name)) {
          instrument(&I, EventType::FINALIZE);
          DidInstrument = true;
          llvm::outs() << "Instrumented " << Name << " call in " << F.getName() << "\n";
        } else if (isOMPFork(Name)) {
          instrument(&I, EventType::PAR_REGION_ENTER);
          instrument(getFirstPostCallInsertPt(CI), EventType::PAR_REGION_EXIT);
          DidInstrument = true;
          llvm::outs() << "Instrumented " << Name << " call in " << F.getName() << "\n";
        }
      }
    }
  }

  return DidInstrument;
}
