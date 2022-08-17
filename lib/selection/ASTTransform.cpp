//
// Created by sebastian on 16.08.22.
//

#include "SelectionSpecAST.h"
#include <cassert>

namespace capi {

template<typename Derived, typename Base, typename Del>
std::unique_ptr<Derived, Del>
dynamic_unique_ptr_cast( std::unique_ptr<Base, Del>&& p )
{
  if(Derived *result = dynamic_cast<Derived *>(p.get())) {
    p.release();
    return std::unique_ptr<Derived, Del>(result, std::move(p.get_deleter()));
  }
  return std::unique_ptr<Derived, Del>(nullptr, p.get_deleter());
}

struct TransformVisitor : public ASTVisitor {

  TransformMap transformMap;

  ASTPtr newAST;

  std::vector<std::vector<NodePtr>> cloneStack;

  TransformVisitor(TransformMap replacements) : transformMap(std::move(replacements)){

  }

  NodePtr executeTransform(ASTNode* node) {
    auto it = transformMap.find(node);
    if (it == transformMap.end()) {
      return {};
    }
    return it->second->execute(node);
  }

  std::vector<NodePtr> popChildren() {
    if (cloneStack.empty())
      return {};
    auto children = std::move(cloneStack.back());
    cloneStack.pop_back();
    return children;
  }

  void pushChildren(std::vector<NodePtr> children) {
    cloneStack.push_back(std::move(children));
  }

  void addChild(NodePtr node) {
    if (cloneStack.empty()) {
      return;
    }
    cloneStack.back().push_back(std::move(node));
  }

  void visitAST(SpecAST &specAst) override {
    visitChildren(specAst);
    newAST = std::make_unique<SpecAST>(popChildren());
  }

  void visitDirective(Directive &directive) override {
    if (auto transformed = executeTransform(&directive)) {
      addChild(std::move(transformed));
    } else {
      visitChildren(directive);
      auto newDirective = std::make_unique<Directive>(directive.getName(), popChildren());
      addChild(std::move(newDirective));
    }
  }

  void visitDecl(SelectorDecl &decl) override {
    if (auto transformed = executeTransform(&decl)) {
      addChild(std::move(transformed));
    } else {
        visitChildren(decl);

        auto children = popChildren();
        assert(children.size() == 1);
        auto child = std::move(children.back());
        std::unique_ptr<SelectorDef> def = dynamic_unique_ptr_cast<SelectorDef>(std::move(child));

        assert(def && "Node must be a defition");

        auto newDirective = std::make_unique<SelectorDecl>(decl.getName(), std::move(def));
        addChild(std::move(newDirective));
      }
  }

  void visitDef(SelectorDef &def) override {
    std::string name = selectorDeclName;
    if (name.empty()) {
      // Definition is not part of a declaration -> generate name
      name = nameGen.next();
      // If definition is passed to selector, add ref parameter.
      if (!builderStack.empty()) {
        builderStack.addRef(name);
      }
    } else {
      // Definition is part of a declaration -> reset name for next definition
      selectorDeclName = "";
    }
    builderStack.beginSelector(name, def.getType());
    visitChildren(def);
    if (!builderStack.finalizeSelector(graph)) {
      encounteredError = true;
    }

  }

  void visitRef(SelectorRef &ref) override {
    builderStack.addRef(ref.getIdentifier());
    visitChildren(ref);
  }

  void visitBoolLiteral(Literal<bool> &l) override {
    auto val = l.getValue();
    builderStack.addParam(Param::makeBool(val));
  }

  void visitIntLiteral(Literal<int> &l) override {
    auto val = l.getValue();
    builderStack.addParam(Param::makeInt(val));
  }

  void visitFloatLiteral(Literal<float> &l) override {
    auto val = l.getValue();
    builderStack.addParam(Param::makeFloat(val));
  }

  void visitStringLiteral(Literal<std::string> &l) override {
    auto val = l.getValue();
    builderStack.addParam(Param::makeString(val));
  }
};

ASTPtr ReplacementExecutor::run(SpecAST &in) {
  ASTPtr newAST = std::make_unique<SpecAST>();
  return capi::ASTPtr();
}


}