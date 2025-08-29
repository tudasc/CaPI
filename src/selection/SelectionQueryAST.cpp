//
// Created by sebastian on 14.03.22.
//

#include "capi/selection/SelectionQueryAST.h"

namespace capi {

 void ASTVisitor::visitAST(QueryAST&ast){visitChildren(ast);};
 void ASTVisitor::visitPipelineDecl(PipelineDecl&decl){visitChildren(decl);};
 void ASTVisitor::visitDef(SelectorDef &def){visitChildren(def);};
 void ASTVisitor::visitRef(PipelineRef&ref){visitChildren(ref);};
 void ASTVisitor::visitInputTuple(PipelineExprTuple&tuple){visitChildren(tuple);};
 void ASTVisitor::visitDirective(Directive& directive){ visitChildren(directive);}
 void ASTVisitor::visitPipelineExpr(PipelineExpr& pipeline) { visitChildren(pipeline);}

void ASTVisitor::visitChildren(ASTNode& node) {
  for (auto &child : node) {
    child.accept(*this);
  }
}


}
