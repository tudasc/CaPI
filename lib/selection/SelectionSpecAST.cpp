//
// Created by sebastian on 14.03.22.
//

#include "SelectionSpecAST.h"

namespace capi {

 void ASTVisitor::visitAST(SpecAST &ast){visitChildren(ast);};
 void ASTVisitor::visitDecl(SelectorDecl &decl){visitChildren(decl);};
 void ASTVisitor::visitDef(SelectorDef &def){visitChildren(def);};
 void ASTVisitor::visitRef(SelectorRef &ref){visitChildren(ref);};
 void ASTVisitor::visitDirective(Directive& directive){ visitChildren(directive);}

void ASTVisitor::visitChildren(ASTNode& node) {
  for (auto &child : node) {
    child.accept(*this);
  }
}


}
