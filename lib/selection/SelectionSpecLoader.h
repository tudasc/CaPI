//
// Created by sebastian on 28.10.21.
//

#ifndef CAPI_CONFIGLOADER_H
#define CAPI_CONFIGLOADER_H

#include "Selector.h"
#include "nlohmann/json.hpp"
#include <string>
using json = nlohmann::json;

class SelectionSpecLoader {
  std::string filename;

public:
  SelectionSpecLoader(const std::string &filename);

  SelectorPtr buildSelector();
};

#endif // CAPI_CONFIGLOADER_H
