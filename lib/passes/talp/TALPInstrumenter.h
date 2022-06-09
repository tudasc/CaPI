//
// Created by sebastian on 07.06.22.
//

#ifndef CAPI_TALPINSTRUMENTER_H
#define CAPI_TALPINSTRUMENTER_H

#include "llvm/Pass.h"

#include <string>

namespace llvm {
class Module;
class Function;
class AnalysisUsage;
} // namespace llvm


class TALPInstrumenter : public llvm::FunctionPass {

public:
  static char ID; // used to identify pass

  TALPInstrumenter();
  bool doInitialization(llvm::Module &) override;
  bool runOnFunction(llvm::Function &) override;
  bool doFinalization(llvm::Module &) override;
  void getAnalysisUsage(llvm::AnalysisUsage &) const override;

private:

};

#endif // CAPI_TALPINSTRUMENTER_H
