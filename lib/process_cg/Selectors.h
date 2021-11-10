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

    inline std::unique_ptr<CallPathSelector<TraverseDir::TraverseUp>> onCallPathTo(SelectorPtr in) {
        return std::make_unique<CallPathSelector<TraverseDir::TraverseUp>>(std::move(in));
    }

    inline std::unique_ptr<CallPathSelector<TraverseDir::TraverseDown>> onCallPathFrom(SelectorPtr in) {
        return std::make_unique<CallPathSelector<TraverseDir::TraverseDown>>(std::move(in));
    }
}