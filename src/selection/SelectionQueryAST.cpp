//
// Created by sebastian on 14.03.22.
//

#include "capi/selection/SelectionQueryAST.h"

namespace capi {

PipelineExpr::PipelineExpr(TermPtr term) : definesSelectors(false) {
  addChild(std::move(term));
}

PipelineExpr::PipelineExpr(PipelineExprPtr inputPipeline, SelectorDefPtr selectorDef) : definesSelectors(true) {
  addChild(std::move(inputPipeline));
  addChild(std::move(selectorDef));
}

 void ASTVisitor::visitAST(QueryAST&ast){visitChildren(ast);};
 void ASTVisitor::visitPipelineDecl(PipelineDecl&decl){visitChildren(decl);};
 void ASTVisitor::visitSelectorDef(SelectorDef &def){visitChildren(def);};
 void ASTVisitor::visitRef(PipelineRef&ref){visitChildren(ref);};
 void ASTVisitor::visitOpTuple(ExprTuple&tuple){visitChildren(tuple);};
 void ASTVisitor::visitDirective(Directive& directive){ visitChildren(directive);}
 void ASTVisitor::visitPipelineExpr(PipelineExpr& pipeline) { visitChildren(pipeline);}
 void ASTVisitor::visitPipelineOp(PipelineOp& op) { visitChildren(op);}
 void ASTVisitor::visitTerm(Term& term) { visitChildren(term);};

 void ASTVisitor::visitChild(ASTNode& node, int idx) {
    auto childIt = node.begin() + idx;
    (*childIt)->accept(*this);
 }


void ASTVisitor::visitChildren(ASTNode& node) {
  for (auto &child : node) {
    child.accept(*this);
  }
}


}
