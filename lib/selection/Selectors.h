//
// Created by sebastian on 29.10.21.
//

#include "Selector.h"

namespace selector {
    inline std::unique_ptr<EverythingSelector> all() {
        return std::make_unique<EverythingSelector>();
    }

    inline std::unique_ptr<NameSelector> byName(std::string regex, SelectorPtr in) {
        return std::make_unique<NameSelector>(std::move(in), std::move(regex));
    }

    inline std::unique_ptr<IncludeListSelector> byIncludeList(std::vector<std::string> includeList, SelectorPtr in) {
        return std::make_unique<IncludeListSelector>(std::move(in), std::move(includeList));
    }

    inline std::unique_ptr<IncludeListSelector> byExcludeList(std::vector<std::string> excludeList, SelectorPtr in) {
        return std::make_unique<IncludeListSelector>(std::move(in), std::move(excludeList));
    }

    inline std::unique_ptr<CallPathSelector<TraverseDir::TraverseUp>> onCallPathTo(SelectorPtr in) {
        return std::make_unique<CallPathSelector<TraverseDir::TraverseUp>>(std::move(in));
    }

    inline std::unique_ptr<CallPathSelector<TraverseDir::TraverseDown>> onCallPathFrom(SelectorPtr in) {
        return std::make_unique<CallPathSelector<TraverseDir::TraverseDown>>(std::move(in));
    }

    inline std::unique_ptr<UnresolvedCallSelector> containingUnresolvedCall(SelectorPtr in) {
        return std::make_unique<UnresolvedCallSelector>(std::move(in));
    }

    inline std::unique_ptr<SetOperationSelector<SetOperation::UNION>> join(SelectorPtr inA, SelectorPtr inB) {
        return std::make_unique<SetOperationSelector<SetOperation::UNION>>(std::move(inA), std::move(inB));
    }

    inline std::unique_ptr<SetOperationSelector<SetOperation::INTERSECTION>> intersect(SelectorPtr inA, SelectorPtr inB) {
        return std::make_unique<SetOperationSelector<SetOperation::INTERSECTION>>(std::move(inA), std::move(inB));
    }

    inline std::unique_ptr<SetOperationSelector<SetOperation::COMPLEMENT>> subtract(SelectorPtr inA, SelectorPtr inB) {
        return std::make_unique<SetOperationSelector<SetOperation::COMPLEMENT>>(std::move(inA), std::move(inB));
    }

    inline std::unique_ptr<SystemIncludeSelector> inSystemInclude(SelectorPtr in) {
        return std::make_unique<SystemIncludeSelector>(std::move(in));
    }

    inline std::unique_ptr<FilePathSelector> byPath(std::string regex, SelectorPtr in) {
        return std::make_unique<FilePathSelector>(std::move(in), std::move(regex));
    }

    inline std::unique_ptr<InlineSelector> inlineSpecified(SelectorPtr in) {
        return std::make_unique<InlineSelector>(std::move(in));
    }


}