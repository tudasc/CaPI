//
// Created by sebastian on 14.03.22.
//

#include "SelectionSpecAST.h"

namespace capi {

void ASTVisitor::traversePreOrder(ASTNode &node) {
  node.accept(*this);
  for (auto &&child : node) {
    traversePreOrder(child)
  }
}

}
