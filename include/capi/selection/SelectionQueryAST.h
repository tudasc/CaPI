//
// Created by sebastian on 14.03.22.
//

#ifndef CAPI_SELECTIONQUERYAST_H
#define CAPI_SELECTIONQUERYAST_H

#include <cassert>
#include <vector>
#include <memory>
#include <iostream>
#include <algorithm>

#include "capi/support/IteratorUtils.h"

namespace capi {

class ASTNode;
template <typename T> class Literal;
class SelectorDef;
class PipelineRef;
class ExprTuple;
class PipelineDecl;
class PipelineOp;
class Term;
class PipelineExpr;
class Directive;
class QueryAST;

class ASTVisitor;

using NodePtr = std::unique_ptr<ASTNode>;
using DirectivePtr = std::unique_ptr<Directive>;
using PipelineDeclPtr = std::unique_ptr<PipelineDecl>;
using SelectorDefPtr = std::unique_ptr<SelectorDef>;
using PipelineRefPtr = std::unique_ptr<PipelineRef>;
using PipelineOpPtr = std::unique_ptr<PipelineOp>;
using TermPtr = std::unique_ptr<Term>;
using ExprTuplePtr = std::unique_ptr<ExprTuple>;
using PipelineExprPtr = std::unique_ptr<PipelineExpr>;
using ASTPtr = std::unique_ptr<QueryAST>;

class ASTVisitor {
public:
  virtual void visitAST(QueryAST &ast);
  virtual void visitPipelineDecl(PipelineDecl &decl);
  virtual void visitSelectorDef(SelectorDef &def);
  virtual void visitRef(PipelineRef&ref);
  virtual void visitOpTuple(ExprTuple&tuple);
  virtual void visitDirective(Directive &directive);
  virtual void visitPipelineExpr(PipelineExpr& pipeline);
  virtual void visitPipelineOp(PipelineOp& op);
  virtual void visitTerm(Term& term);

#define VISIT_LITERAL(name, type) virtual void visit##name##Literal(Literal<type> &l) {}

  VISIT_LITERAL(Bool, bool);
  VISIT_LITERAL(Int, int);
  VISIT_LITERAL(Float, float)
  VISIT_LITERAL(String, std::string);

#undef VISIT_LITERAL

  void visitChildren(ASTNode& node);
  void visitChild(ASTNode& node, int idx);
};

class ASTNode {

protected:
  std::vector<NodePtr> children;

  void addChild(NodePtr ptr) { children.emplace_back(std::move(ptr)); }

  bool addChildAfter(NodePtr ptr, ASTNode* insertAfter) {
    auto insertPos = children.begin();
    bool insert = false;
    while (insertPos != children.end()) {
      if (insertPos->get() == insertAfter) {
        insert = true;
        break;
      }
      ++insertPos;
    }
    if (insert) {
      children.insert(insertPos+1, std::move(ptr));
    }
    return insert;
  }

  template <typename Iterator> void addChildren(Iterator begin, Iterator end) {
    std::for_each(begin, end, [this](auto &child) {
      children.emplace_back(std::move(child));
    });
  }

public:
  virtual ~ASTNode() = default;

  virtual void accept(ASTVisitor &visitor) = 0;

  decltype(dereference_iterator(children.begin())) begin() { return dereference_iterator(children.begin()); }

  decltype(dereference_iterator(children.end())) end() { return dereference_iterator(children.end()); }

  decltype(dereference_iterator(children.cbegin())) cbegin() const { return dereference_iterator(children.cbegin()); }

  decltype(dereference_iterator(children.cend())) cend() const { return dereference_iterator(children.cend()); }

  decltype(children) &getChildren() { return children; }

  const decltype(children) &getChildren() const {return children; }

  NodePtr removeChild(ASTNode* child) {
    auto it = std::find_if(children.begin(), children.end(), [&child](NodePtr& n) {return n && n.get() == child;});
    if (it != children.end()) {
      auto node = std::move(*it);
      children.erase(it);
      return node;
    }
    return nullptr;
  }

  size_t getNumChildren() const {
    return children.size();
  }

  void dumpChildren(std::ostream &os) {
    dumpChildren(os, 0, children.size());
  }

  void dumpChild(std::ostream &os, int idx) {
    auto &child = children[idx];
    child->dump(os);
  }

  void dumpChildren(std::ostream &os, int start, int end) {
    os << "{";
    for (auto i = start; i < end; i++) {
      dumpChild(os, i);
      if (i < end - 1) {
        os << ", ";
      }
    }
    os << "}";
  }

  virtual std::string getTypeName() const {
    return "ASTNode";
  }

  virtual void dump(std::ostream &) = 0;
};

template <typename T> class Literal : public ASTNode {
protected:
  T val;

public:
  Literal() = default;
  explicit Literal(T val) : val(std::move(val)) {}

  T getValue() const { return val; }

  void dump(std::ostream &os) override {
    os << "<Literal> {value='" << val << "'}";
  }

  std::string getTypeName() const override {
    return "Literal";
  }
};

class BoolLiteral : public Literal<bool> {
public:
  explicit BoolLiteral(bool val) : Literal<bool>(val){}

  static std::unique_ptr<BoolLiteral> fromString(std::string_view str) {
    if (str == "false") {
      return std::make_unique<BoolLiteral>(false);
    }
    if (str == "true") {
      return std::make_unique<BoolLiteral>(true);
    }
    return {};
  }

  void accept(ASTVisitor &visitor) override {
    visitor.visitBoolLiteral(*this);
  }
};

class IntLiteral : public Literal<int> {
public:
  explicit IntLiteral(int val) : Literal<int>(val){}

  static std::unique_ptr<IntLiteral> fromString(std::string_view str) {
    int val = std::stoi(std::string(str)); // FIXME: May throw exception
    return std::make_unique<IntLiteral>(val);
  }

  void accept(ASTVisitor &visitor) override {
    visitor.visitIntLiteral(*this);
  }
};

class FloatLiteral : public Literal<float> {
public:
  explicit FloatLiteral(float val) : Literal<float>(val){}

  static std::unique_ptr<FloatLiteral> fromString(std::string_view str) {
    float val = std::stof(std::string(str));
    return std::make_unique<FloatLiteral>(val);
  }

  void accept(ASTVisitor &visitor) override {
    visitor.visitFloatLiteral(*this);
  }
};

class StringLiteral : public Literal<std::string> {
public:
  explicit StringLiteral(std::string val) : Literal<std::string>(val){}

  void accept(ASTVisitor &visitor) override {
    visitor.visitStringLiteral(*this);
  }
};

class PipelineRef : public ASTNode {
  std::string identifier;

public:
  explicit PipelineRef(std::string identifier)
      : identifier(std::move(identifier)) {}

  std::string getIdentifier() const { return identifier; }

  void accept(ASTVisitor &visitor) override { visitor.visitRef(*this); }

  void dump(std::ostream &os) override {
    os << "<SelectorRef> {selector=%" << identifier << "}";
  }
  std::string getTypeName() const override {
    return "PipelineRef";
  }
};

class Directive : public ASTNode {

  std::string name;

public:
  using Params = std::vector<NodePtr>;

  Directive(std::string name, Params params)
      : name(std::move(name)) {
    addChildren(params.begin(), params.end());
  }

  std::string getDirectiveName() const {
    return name;
  }

  std::string getTypeName() const override {
    return getDirectiveName() + " directive";
  }

  void accept(ASTVisitor &visitor) override {
    visitor.visitDirective(*this);
  }

  void dump(std::ostream &os) override {
    os << "<Directive> {type=" << name << ", params=";
    dumpChildren(os);
    os << "}";
  }
};


class SelectorDef : public ASTNode {

  std::string selectorType;

public:
  using Params = std::vector<NodePtr>;

  SelectorDef(std::string selectorType, Params params)
      : selectorType(std::move(selectorType)) {
    addChildren(params.begin(), params.end());
  }

  std::string getType() {
    return selectorType;
  }

  std::string getTypeName() const override {
    return "SelectorDef";
  }

  void accept(ASTVisitor &visitor) override { visitor.visitSelectorDef(*this); }

  void dump(std::ostream &os) override {
    os << "<SelectorDef> {selector=" << selectorType << ", params=";
    dumpChildren(os);
    os << "}";
  }
};

class ExprTuple : public ASTNode {
 public:
  explicit ExprTuple(std::vector<PipelineOpPtr> inputs) {
    addChildren(inputs.begin(), inputs.end());
  }


  void accept(ASTVisitor &visitor) override { visitor.visitOpTuple(*this);
  }

  void dump(std::ostream &os) override {
    os << "<ExprTuple> {tuple=";
    dumpChildren(os);
    os << "}";
  }

  std::string getTypeName() const override {
    return "ExprTuple";
  }
};

enum class OperatorType {
  ID, UNION, INTERSECT, DIFF
};

inline std::string getOperatorName(OperatorType op) {
  switch(op) {
    case OperatorType::ID:
      return "ID";
    case OperatorType::UNION:
      return "union";
    case OperatorType::INTERSECT:
      return "intersection";
    case OperatorType::DIFF:
      return "difference";
    default:
      break;
  }
  assert(false && "Unhandled op type");
}

class PipelineExpr : public ASTNode {
  bool definesSelectors;
  bool pipelineIsAlias;
 public:
  explicit PipelineExpr(TermPtr term);

  explicit PipelineExpr(PipelineExprPtr inputPipeline, SelectorDefPtr selectorDef);

  bool doesDefineSelectors() const {
    return definesSelectors;
  }

  bool isAlias() const {
    return pipelineIsAlias;
  }

  void accept(ASTVisitor &visitor) override { visitor.visitPipelineExpr(*this); }

  void dump(std::ostream &os) override {
    os << "<PipelineExpr> {input=";
    dumpChild(os, 0);
    if (definesSelectors) {
      os << ", def=";
      dumpChild(os, 1);
    }
    os << "}";
  }

  std::string getTypeName() const override {
    return "PipelineExpr";
  }
};




class PipelineOp : public ASTNode {
  OperatorType type;
 public:
  explicit PipelineOp(PipelineOpPtr lhs, PipelineExprPtr rhs, OperatorType type) : type(type) {
    addChild(std::move(lhs));
    addChild(std::move(rhs));
  }
  explicit PipelineOp(PipelineExprPtr lhs) : type(OperatorType::ID) {
    addChild(std::move(lhs));
  }

  OperatorType getOperatorType() const {
    return type;
  }

  void accept(ASTVisitor &visitor) override { visitor.visitPipelineOp(*this); }

  void dump(std::ostream &os) override {
    os << "<PipelineOp> {op=" << getOperatorName(type);
    os << ", lhs=";
    dumpChild(os, 0);
    if (type != OperatorType::ID) {
      os << ", rhs=";
      dumpChild(os, 1 );
    }
    os << "}";
  }

  std::string getTypeName() const override {
    return "PipelineOp";
  }

};


class Term : public ASTNode {

  enum TermKind {
    TUPLE, REF, EXPR
  };

  TermKind kind;

 public:
  explicit Term(PipelineRefPtr ref) : kind(REF) {
    addChild(std::move(ref));
  }
  explicit Term(PipelineOpPtr op) : kind(EXPR)  {
    addChild(std::move(op));
  }
  explicit Term(ExprTuplePtr tuple) : kind(TUPLE) {
    addChild(std::move(tuple));
  }

  bool isTuple() const {
    return kind == TUPLE;
  }

  bool isRef() const {
    return kind == REF;
  }

  void accept(ASTVisitor &visitor) override { visitor.visitTerm(*this); }

  void dump(std::ostream &os) override {
    dumpChild(os, 0);
  }

  std::string getTypeName() const override {
    return "PipelineOperand";
  }

};



//class PipelineInput : public ASTNode {
//  bool inputIsExpr;
// public:
//  explicit PipelineInput(PipelineRefPtr ref) : inputIsExpr(false) {
//    addChild(std::move(ref));
//  }
//  explicit PipelineInput(PipelineExprPtr expr) : inputIsExpr(true) {
//    addChild(std::move(expr));
//  }
//
//  bool isInputExpr() const {
//    return inputIsExpr;
//  }
//
//  bool isInputRef() const {
//    return !inputIsExpr;
//  }
//
//  void accept(ASTVisitor &visitor) override {
//    visitor.visitPipelineInput(*this);
//  }
//
//  void dump(std::ostream &os) override {
//    os << "<PipelineInput> value=";
//    dumpChildren(os);
//  }
//};


class PipelineDecl : public ASTNode {
  std::string identifier;

public:
 PipelineDecl(std::string identifier, PipelineOpPtr pipeline)
      : identifier(std::move(identifier)) {
    addChild(std::move(pipeline));
  }

  explicit PipelineDecl(PipelineOpPtr pipeline)
      : PipelineDecl("", std::move(pipeline)) {}

  std::string getName() const {
    return identifier;
  }

  void accept(ASTVisitor &visitor) override { visitor.visitPipelineDecl(*this); }

  void dump(std::ostream &os) override {
    os << "<SelectorDecl> {name=" << identifier << ", def=";
    dumpChild(os, 0);
    os << "}";
  }

  std::string getTypeName() const override {
    return "PipelineDecl";
  }
};

class QueryAST : public ASTNode {
public:
  explicit QueryAST(std::vector<NodePtr> stmts) {
    addChildren(stmts.begin(), stmts.end());
  }

  void accept(ASTVisitor &visitor) override { visitor.visitAST(*this); }

  void dump(std::ostream &os) override {
    os << "<QueryAST> {stmts=";
    dumpChildren(os);
    os << "}";
  }

  bool insertStmt(NodePtr stmt, ASTNode* insertAfter) {
    return addChildAfter(std::move(stmt), insertAfter);
  }

  std::string getTypeName() const override {
    return "QueryAST";
  }

};

inline ASTNode* findParent(ASTNode& root, ASTNode& node) {
  for (auto& child : root.getChildren()) {
    if (child.get() == &node) {
      return &root;
    }
    auto childParent = findParent(*child, node);
    if (childParent) {
      return childParent;
    }
  }
  return nullptr;
}

}


#endif  // CAPI_SELECTIONQUERYAST_H
