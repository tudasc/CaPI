//
// Created by sebastian on 14.10.21.
//

#ifndef CAPI_READMETACG_H
#define CAPI_READMETACG_H

#include "llvm/Pass.h"
#include "MetaCGReader.h"

namespace llvm {
    class Module;
    class Function;
    class AnalysisUsage;
}  // namespace llvm


class ReadMetaCG : public llvm::FunctionPass {

public:
    static char ID;  // used to identify pass

    ReadMetaCG();
    bool doInitialization(llvm::Module&) override;
    bool runOnFunction(llvm::Function&) override;
    bool doFinalization(llvm::Module&) override;
    void getAnalysisUsage(llvm::AnalysisUsage&) const override;

private:
    MetaCGReader::FInfoMap functionInfo;

};


#endif //CAPI_READMETACG_H
