//
// Created by sebastian on 28.10.21.
//

#include "DOTWriter.h"
#include "Selector.h"


#include <ostream>

namespace capi {

static inline std::string getNodeId(CGNode &node)
{
  return "n_" + std::to_string(reinterpret_cast<uintptr_t>(&node));
}

bool writeDOT(const CallGraph &cg, const FunctionFilter& filter, std::ostream &out)
{
  out << "digraph {\n";
  for (auto &node : cg.getNodes()) {
    if (filter.accepts(node->getName()))
      out << getNodeId(*node) << " [label=\"" << node->getName() << "\"]\n";
  }

  for (auto &node : cg.getNodes()) {
    if (filter.accepts(node->getName())) {
      for (auto &callee : node->findAllCallees()) {
        if (filter.accepts(callee->getName())) {
          out << getNodeId(*node) << " -> " << getNodeId(*callee) << "\n";
        }
      }
    }
  }
  out << "}\n";
  return true;
}

}