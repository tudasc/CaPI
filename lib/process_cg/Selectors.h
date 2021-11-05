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

    inline std::unique_ptr<CallPathSelector> onCallPathTo(SelectorPtr in) {
        return std::make_unique<CallPathSelector>(std::move(in));
    }
}