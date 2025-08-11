//
// Created by sebastian on 28.10.21.
//

#include "capi/selection/DOTWriter.h"
#include "capi/selection/Selector.h"
#include "capi/selection/TraversalHelper.h"


#include <ostream>

namespace capi {

const std::string NodeDecoration::ColorStrings[] = {
    "red", "blue", "green", "yellow", "black", "white"
};

static inline std::string getNodeId(const metacg::CgNode &node)
{
  return std::to_string(node.getId());
}

static inline std::string getNodeAttrs(NodeDecoration deco) {
  std::stringstream attrs;
  bool addComma{false};
  if (deco.shapeColor != NodeDecoration::UNSPECIFIED) {
    attrs << "color=" << NodeDecoration::ColorStrings[deco.shapeColor];
    addComma = true;
  }
  if (deco.textColor != NodeDecoration::UNSPECIFIED) {
    if (addComma) {
      attrs << ", ";
    }
    attrs << "fontcolor=" << NodeDecoration::ColorStrings[deco.textColor];
    addComma = true;
  }
  if (deco.bgColor != NodeDecoration::UNSPECIFIED) {
    if (addComma) {
      attrs << ", ";
    }
    attrs << "bgcolor=" << NodeDecoration::ColorStrings[deco.bgColor];
    addComma = true;
  }
  return attrs.str();
}

bool writeDOT(TraversalHelper &helper, const FunctionFilter& filter, const DecorationMap& decoration, std::ostream &out) {

  auto getDeco = [&decoration](const std::string& name) -> NodeDecoration {
    if (auto entry = decoration.find(name); entry != decoration.end()) {
      return entry->second;
    }
    return {};
  };

  bool acceptAll = filter.size() == 0;

  out << "digraph {\n";
  for (auto& node: helper.cg.getNodes()) {

    if (acceptAll || filter.accepts(node->getFunctionName())) {
      auto attrStr = getNodeAttrs(getDeco(node->getFunctionName()));
      out << getNodeId(*node) << " [label=\"" << node->getFunctionName() << (attrStr.empty() ? "\"" : "\", ") << attrStr << "]\n";
    }
  }

  for (auto& node : helper.cg.getNodes()) {
    if (acceptAll || filter.accepts(node->getFunctionName())) {
      for (auto &callee : helper.get(node.get()).findAllCallees()) {
        if (acceptAll || filter.accepts(callee->getFunctionName())) {
          out << getNodeId(*node) << " -> " << getNodeId(*callee) << "\n";
        }
      }
    }
  }
  out << "}\n";
  return true;
}

}