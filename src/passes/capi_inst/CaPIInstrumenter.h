//
// Created by sebastian on 14.10.21.
//

#ifndef CAPI_APPLYINSTRUMENTATIONFILTER_H
#define CAPI_APPLYINSTRUMENTATIONFILTER_H

#include "llvm/Pass.h"
#include "llvm/IR/PassManager.h"

#include <string>

namespace llvm {
class Module;
class Function;
class AnalysisUsage;
} // namespace llvm

enum class InstAPI {
  GNU, TALP
};


class CaPIInstrumenter : public llvm::PassInfoMixin<CaPIInstrumenter> {

public:

  CaPIInstrumenter();

  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &);

private:
  bool runOnFunction(llvm::Function &);

private:
  std::vector<std::string> filterList;
  InstAPI api;
};
//
//class CaPIInstrumenter : public llvm::FunctionPass {
//
//public:
//  static char ID; // used to identify pass
//
//  CaPIInstrumenter();
//  bool doInitialization(llvm::Module &) override;
//  bool runOnFunction(llvm::Function &) override;
//  bool doFinalization(llvm::Module &) override;
//  void getAnalysisUsage(llvm::AnalysisUsage &) const override;
//
//private:
//  std::vector<std::string> filterList;
//  InstAPI api;
//};

#endif // CAPI_APPLYINSTRUMENTATIONFILTER_H
