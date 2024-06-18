//
// Created by sebastian on 28.10.21.
//

#ifndef CAPI_CGTRAITS_H
#define CAPI_CGTRAITS_H

#include "CallGraph.h"
#include "Selector.h"
#include "FunctionFilter.h"

namespace capi {

struct NodeDecoration {
  enum Color {
    RED,
    BLUE,
    GREEN,
    YELLOW,
    BLACK,
    WHITE,
    UNSPECIFIED
  };
  static const std::string ColorStrings[];

  Color textColor{UNSPECIFIED};
  Color shapeColor{UNSPECIFIED};
  Color bgColor{UNSPECIFIED};
};

using DecorationMap = std::unordered_map<std::string, NodeDecoration>;

bool writeDOT(const CallGraph &cg, const FunctionFilter& selection, const DecorationMap& decoration, std::ostream &out);

}

#endif // CAPI_CGTRAITS_H
