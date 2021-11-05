//
// Created by sebastian on 14.10.21.
//

#ifndef INSTCONTROL_APPLYINSTRUMENTATIONFILTER_H
#define INSTCONTROL_APPLYINSTRUMENTATIONFILTER_H

#include "llvm/Pass.h"

#include <string>

namespace llvm {
    class Module;
    class Function;
    class AnalysisUsage;
}  // namespace llvm


class ApplyInstrumentationFilter : public llvm::FunctionPass {

public:
    static char ID;  // used to identify pass

    ApplyInstrumentationFilter();
    bool doInitialization(llvm::Module&) override;
    bool runOnFunction(llvm::Function&) override;
    bool doFinalization(llvm::Module&) override;
    void getAnalysisUsage(llvm::AnalysisUsage&) const override;

private:
    std::vector<std::string> filterList;

};


#endif //INSTCONTROL_APPLYINSTRUMENTATIONFILTER_H
