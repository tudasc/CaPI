//
// Created by sebastian on 12.08.25.
//
#ifndef CAPI_NESMIKPASS_H
#define CAPI_NESMIKPASS_H

#include "llvm/Pass.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Value.h"

#include <string>

namespace llvm {
class Module;
class Function;
class AnalysisUsage;
} // namespace llvm

class NeSmiKPass : public llvm::PassInfoMixin<NeSmiKPass> {

 public:
  NeSmiKPass() = default;

  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &);

 private:
  bool runOnFunction(llvm::Function &);

  llvm::Value* InitEventStr;
  llvm::Value* ExitEventStr;
  llvm::Value* ParRegionEnterEventStr;
  llvm::Value* ParRegionExitEventStr;

};

#endif  // CAPI_NESMIKPASS_H
