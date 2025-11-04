//
// Created by sebastian on 29.08.25.
//

#ifndef CAPI_ASTDOTEXPORTER_H
#define CAPI_ASTDOTEXPORTER_H

#include <fstream>
#include <string>
#include <unordered_map>

#include "capi/selection/SelectionQueryAST.h"

namespace capi {

class ASTDotExporter : public ASTVisitor {
  std::ostream &os;
  int nextId = 0;
  std::unordered_map<const ASTNode*, int> ids;

  int getId(const ASTNode *node) {
    auto it = ids.find(node);
    if (it != ids.end()) return it->second;
    int id = nextId++;
    ids[node] = id;
    return id;
  }

  void emitNode(const ASTNode *node, const std::string &label) {
    int id = getId(node);
    os << "  node" << id
       << " [label=\"" << escape(label) << "\", shape=box, style=rounded];\n";

    for (auto &child : node->getChildren()) {
      int cid = getId(child.get());
      os << "  node" << id << " -> node" << cid << ";\n";
      // recurse via visitor
      child->accept(*this);
    }
  }

  static std::string escape(const std::string &s) {
    std::string out;
    for (char c : s) {
      if (c == '"' || c == '\\') out.push_back('\\');
      out.push_back(c);
    }
    return out;
  }

 public:
  ASTDotExporter(std::ostream &os) : os(os) {}

  void exportAST(ASTNode &root) {
    os << "digraph AST {\n";
    os << "  node [shape=box, style=rounded];\n";
    root.accept(*this);
    os << "}\n";
  }

  // ---- visitor overrides ----

  void visitAST(QueryAST &ast) override {
    emitNode(&ast, "QueryAST");
  }

  void visitPipelineDecl(PipelineDecl &decl) override {
    emitNode(&decl, "PipelineDecl " + decl.getName());
  }

  void visitSelectorDef(SelectorDef &def) override {
    emitNode(&def, "SelectorDef " + def.getType());
  }

  void visitRef(PipelineRef &ref) override {
    emitNode(&ref, "PipelineRef: %" + ref.getIdentifier());
  }

  void visitTerm(Term &term) override {
    emitNode(&term, "Term");
  }

  void visitOpTuple(ExprTuple&tuple) override {
    emitNode(&tuple, "PipelineExprTuple");
  }

  void visitDirective(Directive &directive) override {
    emitNode(&directive, "Directive " + directive.getDirectiveName());
  }

  void visitPipelineExpr(PipelineExpr &pipeline) override {
    emitNode(&pipeline, "PipelineExpr");
  }

  void visitPipelineOp(PipelineOp &op) override {
    emitNode(&op, "PipelineOp " + getOperatorName(op.getOperatorType()));
  }

  // literals
  void visitBoolLiteral(Literal<bool> &l) override {
    emitNode(&l, std::string("BoolLiteral: ") + (l.getValue() ? "true" : "false"));
  }
  void visitIntLiteral(Literal<int> &l) override {
    emitNode(&l, "IntLiteral: " + std::to_string(l.getValue()));
  }
  void visitFloatLiteral(Literal<float> &l) override {
    std::ostringstream ss;
    ss << l.getValue();
    emitNode(&l, "FloatLiteral: " + ss.str());
  }
  void visitStringLiteral(Literal<std::string> &l) override {
    emitNode(&l, "StringLiteral: \"" + escape(l.getValue()) + "\"");
  }
};

}

#endif  // CAPI_ASTDOTEXPORTER_H
