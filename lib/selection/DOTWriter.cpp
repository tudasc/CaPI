//
// Created by sebastian on 28.10.21.
//

#include "DOTWriter.h"
#include "Selector.h"


#include <ostream>

namespace capi {

const std::string NodeDecoration::ColorStrings[] = {
    "red", "blue", "green", "yellow", "black", "white"
};

static inline std::string getNodeId(CGNode &node)
{
  return "n_" + std::to_string(reinterpret_cast<uintptr_t>(&node));
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

bool writeDOT(const CallGraph &cg, const FunctionFilter& filter, const DecorationMap& decoration, std::ostream &out) {

  auto getDeco = [&decoration](const std::string& name) -> NodeDecoration {
    if (auto entry = decoration.find(name); entry != decoration.end()) {
      return entry->second;
    }
    return {};
  };

  bool acceptAll = filter.size() == 0;

  out << "digraph {\n";
  for (auto &node : cg.getNodes()) {

    if (acceptAll || filter.accepts(node->getName())) {
      auto attrStr = getNodeAttrs(getDeco(node->getName()));
      out << getNodeId(*node) << " [label=\"" << node->getName() << (attrStr.empty() ? "\"" : "\", ") << attrStr << "]\n";
    }
  }

  for (auto &node : cg.getNodes()) {
    if (acceptAll || filter.accepts(node->getName())) {
      auto all = node->getCallees();
      for (auto &callee : node->findAllCallees()) {
        if (acceptAll || filter.accepts(callee->getName())) {
          out << getNodeId(*node) << " -> " << getNodeId(*callee) << "\n";
        }
      }
    }
  }
  out << "}\n";
  return true;
}

}