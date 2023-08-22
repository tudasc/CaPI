//
// Created by sebastian on 14.03.22.
//

#ifndef CAPI_SELECTIONSPECAST_H
#define CAPI_SELECTIONSPECAST_H

#include <vector>
#include <memory>
#include <iostream>
#include <algorithm>

#include "support/IteratorUtils.h"

namespace capi {

class ASTNode;
template <typename T> class Literal;
class SelectorDef;
class SelectorRef;
class SelectorDecl;
class Directive;
class SpecAST;

class ASTVisitor;

using NodePtr = std::unique_ptr<ASTNode>;
using DirectivePtr = std::unique_ptr<Directive>;
using DeclPtr = std::unique_ptr<SelectorDecl>;
using DefPtr = std::unique_ptr<SelectorDef>;
using RefPtr = std::unique_ptr<SelectorRef>;
using ASTPtr = std::unique_ptr<SpecAST>;

class ASTVisitor {
public:
  virtual void visitAST(SpecAST &ast);
  virtual void visitDecl(SelectorDecl &decl);
  virtual void visitDef(SelectorDef &def);
  virtual void visitRef(SelectorRef &ref);
  virtual void visitDirective(Directive &directive);


#define VISIT_LITERAL(name, type) virtual void visit##name##Literal(Literal<type> &l) {}

  VISIT_LITERAL(Bool, bool);
  VISIT_LITERAL(Int, int);
  VISIT_LITERAL(Float, float)
  VISIT_LITERAL(String, std::string);

#undef VISIT_LITERAL

  void visitChildren(ASTNode& node);
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

  void dumpChildren(std::ostream &os) {
    os << "{";
    for (auto i = 0; i < children.size(); i++) {
      auto &child = children[i];
      child->dump(os);
      if (i < children.size() - 1) {
        os << ", ";
      }
    }
    os << "}";
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
    os << "<Literal> value='" << val << "'";
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

class SelectorRef : public ASTNode {
  std::string identifier;

public:
  explicit SelectorRef(std::string identifier)
      : identifier(std::move(identifier)) {}

  std::string getIdentifier() const { return identifier; }

  void accept(ASTVisitor &visitor) override { visitor.visitRef(*this); }

  void dump(std::ostream &os) override {
    os << "<SelectorRef> selector=%" << identifier;
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

  std::string getName() {
    return name;
  }

  void accept(ASTVisitor &visitor) override {
    visitor.visitDirective(*this);
  }

  void dump(std::ostream &os) override {
    os << "<Directive> type=" << name << ", params=";
    dumpChildren(os);
  }
};

//class ImportDirective : public Directive {
//public:
//  explicit ImportDirective(Params params) : Directive("ImportDirective", std::move(params)) {
//  }
//
//  void accept(ASTVisitor &visitor) override {
//    visitor.visitImportDirective(*this);
//  }
//
//};


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

  void accept(ASTVisitor &visitor) override { visitor.visitDef(*this); }

  void dump(std::ostream &os) override {
    os << "<SelectorDef> selector=" << selectorType << ", params=";
    dumpChildren(os);
  }
};

class SelectorDecl : public ASTNode {
  std::string identifier;

public:
  SelectorDecl(std::string identifier, std::unique_ptr<SelectorDef> def)
      : identifier(std::move(identifier)) {
    addChild(std::move(def));
  }

  explicit SelectorDecl(std::unique_ptr<SelectorDef> def)
      : SelectorDecl("", std::move(def)) {}

  std::string getName() const {
    return identifier;
  }

  void accept(ASTVisitor &visitor) override { visitor.visitDecl(*this); }

  void dump(std::ostream &os) override {
    os << "<SelectorDecl> name=" << identifier << ", def=";
    dumpChildren(os);
  }
};

class SpecAST : public ASTNode {
public:
  explicit SpecAST(std::vector<NodePtr> stmts) {
    addChildren(stmts.begin(), stmts.end());
  }

  void accept(ASTVisitor &visitor) override { visitor.visitAST(*this); }

  void dump(std::ostream &os) override {
    os << "<SpecAST> stmts=";
    dumpChildren(os);
  }

  bool insertStmt(NodePtr stmt, ASTNode* insertAfter) {
    return addChildAfter(std::move(stmt), insertAfter);
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


#endif // CAPI_SELECTIONSPECAST_H
