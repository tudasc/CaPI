//
// Created by sebastian on 07.06.22.
//

#ifndef CAPI_TALPINSTRUMENTER_H
#define CAPI_TALPINSTRUMENTER_H

#include "llvm/Pass.h"
#include "llvm/IR/PassManager.h"

#include <string>


class TALPInstrumenter : public llvm::PassInfoMixin<TALPInstrumenter> {
public:
  TALPInstrumenter();
  llvm::PreservedAnalyses run(llvm::Function &F, llvm::FunctionAnalysisManager &);
};

//class TALPInstrumenter : public llvm::FunctionPass {
//
//public:
//  static char ID; // used to identify pass
//
//  TALPInstrumenter();
//  bool doInitialization(llvm::Module &) override;
//  bool runOnFunction(llvm::Function &) override;
//  bool doFinalization(llvm::Module &) override;
//  void getAnalysisUsage(llvm::AnalysisUsage &) const override;
//
//private:
//
//};

#endif // CAPI_TALPINSTRUMENTER_H
