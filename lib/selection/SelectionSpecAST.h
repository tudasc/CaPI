//
// Created by sebastian on 14.03.22.
//

#ifndef CAPI_SELECTIONSPECAST_H
#define CAPI_SELECTIONSPECAST_H

#include "Utils.h"

#include <memory>
#include <iostream>
#include <algorithm>

namespace capi {

class ASTNode;
template <typename T> class Literal;
class SelectorDef;
class SelectorRef;
class SelectorDecl;
class SpecAST;

class ASTVisitor;

using NodePtr = std::unique_ptr<ASTNode>;

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

  template <typename Iterator> void addChildren(Iterator begin, Iterator end) {
    std::for_each(begin, end, [this](auto &child) {
      children.emplace_back(std::move(child));
    });
  }

public:
  virtual void accept(ASTVisitor &visitor) = 0;

  decltype(dereference_iterator(children.begin())) begin() { return dereference_iterator(children.begin()); }

  decltype(dereference_iterator(children.end())) end() { return dereference_iterator(children.end()); }

  decltype(children) &getChildren() { return children; }

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
  explicit SpecAST(std::vector<std::unique_ptr<SelectorDecl>> decls) {
    addChildren(decls.begin(), decls.end());
  }

  void accept(ASTVisitor &visitor) override { visitor.visitAST(*this); }

  void dump(std::ostream &os) override {
    os << "<SpecAST> decls=";
    dumpChildren(os);
  }
};

}


#endif // CAPI_SELECTIONSPECAST_H
