//
// Created by sebastian on 07.06.22.
//

#include "TALPInstrumenter.h"


#include "llvm/IR/Instructions.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/IRBuilder.h"


#include <fstream>
#include <string>

using namespace llvm;

namespace {
static llvm::RegisterPass<TALPInstrumenter>
    passreg("talp-inst", "Apply TALP region instrumentation", false,
            false);
}

static StructType* getMonitorStructType(Module& M) {
  auto MonitorTy = M.getTypeByName("dlb_monitor_t");
  if (MonitorTy) {
    return MonitorTy;
  }
  return StructType::create(M.getContext(), "dlb_monitor_t");
}

static Value* insertRegionEnter(Function &F, Instruction* InsertionPt, DebugLoc DL) {
  Module &M = *InsertionPt->getParent()->getParent()->getParent();
  LLVMContext &C = InsertionPt->getParent()->getContext();

  auto MonitorTy = getMonitorStructType(M);

  auto MonitorPtrTy = PointerType::get(MonitorTy, 0);

  FunctionCallee RegisterFn = M.getOrInsertFunction("DLB_MonitoringRegionRegister", MonitorPtrTy, Type::getInt8PtrTy(C));

  FunctionCallee StartFn = M.getOrInsertFunction("DLB_MonitoringRegionStart", Type::getInt32Ty(C), MonitorPtrTy);

  IRBuilder<> IRB(InsertionPt);
  auto RegionName = IRB.CreateGlobalStringPtr(F.getName());
  auto RegionNamePtr = IRB.CreateInBoundsGEP(IRB.getInt8PtrTy(), RegionName, ConstantInt::get(IRB.getInt64Ty(), 0, false));
  auto RegisterCall = IRB.CreateCall(RegisterFn, {RegionNamePtr});
  auto RegionHandle = IRB.CreateBitOrPointerCast(RegisterCall, MonitorPtrTy);


  auto StartCall = IRB.CreateCall(StartFn, {RegionHandle});

  return RegionHandle;

}

static void insertRegionExit(Function &F, Instruction* InsertionPt, DebugLoc DL, Value* RegionHandle) {
  Module &M = *InsertionPt->getParent()->getParent()->getParent();
  LLVMContext &C = InsertionPt->getParent()->getContext();

  auto MonitorTy = getMonitorStructType(M);
  auto MonitorPtrTy = PointerType::get(MonitorTy, 0);

  FunctionCallee StopFn = M.getOrInsertFunction("DLB_MonitoringRegionStop", Type::getInt32Ty(C), MonitorPtrTy);

  IRBuilder<> IRB(InsertionPt);
  auto StartCall = IRB.CreateCall(StopFn, {RegionHandle});
}

char TALPInstrumenter::ID = 0;

TALPInstrumenter::TALPInstrumenter() : llvm::FunctionPass(ID) {
}

bool TALPInstrumenter::doInitialization(llvm::Module &M) {
  return false;
}

static bool RegionContainsMPIInitOrFinalize(llvm::Function &F) {
  for (auto& BB : F) {
    for (auto& I : BB) {
      if (auto CI = dyn_cast<CallBase>(&I); CI) {
        auto Callee = CI->getCalledFunction()->getName();
        if (Callee == "MPI_Init" || Callee == "MPI_Finalize") {
          return true;
        }
      }
    }
  }
  return false;
}

bool TALPInstrumenter::runOnFunction(llvm::Function &F) {

  // This is partially copied from EntryExitInstrumenter

  StringRef TALPAttr = "instrument-talp-region";
  auto AttrVal = F.getFnAttribute(TALPAttr).getValueAsString();
  bool ShouldInstrument = AttrVal == "true";

  if (!ShouldInstrument) {
    return false;
  }

  if (RegionContainsMPIInitOrFinalize(F)) {
    llvm::outs() << "Unable to create TALP region for " << F.getName() << ": Function is partially outside of MPI scope.\n";
    return false;
  }

  llvm::outs() << "Running TALP region instrumenter on function " << F.getName() << "\n";

  F.removeAttribute(AttributeList::FunctionIndex, TALPAttr);

    DebugLoc DL;
    if (auto SP = F.getSubprogram())
      DL = DebugLoc::get(SP->getScopeLine(), 0, SP);

    auto RegionHandle = insertRegionEnter(F, &*F.begin()->getFirstInsertionPt(), DL);




    for (BasicBlock &BB : F) {
      Instruction *T = BB.getTerminator();
      if (!isa<ReturnInst>(T))
        continue;

      // If T is preceded by a musttail call, that's the real terminator.
      Instruction *Prev = T->getPrevNode();
      if (BitCastInst *BCI = dyn_cast_or_null<BitCastInst>(Prev))
        Prev = BCI->getPrevNode();
      if (CallInst *CI = dyn_cast_or_null<CallInst>(Prev)) {
        if (CI->isMustTailCall())
          T = CI;
      }

      DebugLoc DL;
      if (DebugLoc TerminatorDL = T->getDebugLoc())
        DL = TerminatorDL;
      else if (auto SP = F.getSubprogram())
        DL = DebugLoc::get(0, 0, SP);

      insertRegionExit(F, T, DL, RegionHandle);
    }

  return true;

}

bool TALPInstrumenter::doFinalization(llvm::Module &M) {
  return false;
}

void TALPInstrumenter::getAnalysisUsage(llvm::AnalysisUsage &AU) const {
  AU.setPreservesCFG();
}


static void registerClangPass(const llvm::PassManagerBuilder &,
                              llvm::legacy::PassManagerBase &PM) {
  PM.add(new TALPInstrumenter());
}
static RegisterStandardPasses
    RegisterClangPass(PassManagerBuilder::EP_OptimizerLast,
                      registerClangPass);

