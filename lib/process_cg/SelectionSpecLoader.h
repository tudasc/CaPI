//
// Created by sebastian on 28.10.21.
//

#ifndef INSTCONTROL_CONFIGLOADER_H
#define INSTCONTROL_CONFIGLOADER_H

#include <string>
#include "Selector.h"
#include "nlohmann/json.hpp"
using json = nlohmann::json;

class SelectionSpecLoader {
    std::string filename;
public:
    SelectionSpecLoader(const std::string& filename);

    SelectorPtr buildSelector();

};




#endif //INSTCONTROL_CONFIGLOADER_H
