//
// Created by sebastian on 15.03.22.
//

#include "SelectorBuilder.h"
#include "../Utils.h"
#include "SelectionSpecAST.h"
#include "SelectorRegistry.h"
#include <selectors/BasicSelectors.h>

namespace capi {

namespace {
std::unordered_map<std::string, SelectorFactoryFn> selectorRegistry;
}

RegisterSelector::RegisterSelector(std::string selectorType, SelectorFactoryFn fn) {
  //std::cout << "Registered selector: " << selectorType << "\n";
  selectorRegistry[selectorType] = std::move(fn);
}

class SelectorEmitter: public ASTVisitor {

  struct NameGen {
    explicit NameGen(std::string basename) : basename(std::move(basename)), count(0) {}

    std::string next() {
      return basename + std::to_string(count++);
    }

  private:
    std::string basename;
    int count;
  };


  SpecAST& ast;
  SelectorGraph& graph;

  bool encounteredError{false};

  std::string selectorDeclName;
  std::string lastDeclName;

  NameGen nameGen;

public:

  SelectorEmitter(SpecAST& ast, SelectorGraph& graph) : ast(ast), graph(graph), nameGen("anon_") {
    selectorDeclName = "";
    graph.createNode("%", std::make_unique<EverythingSelector>());
  }

  struct SelectorBuilder {
    std::string name;
    std::string selectorType;
    std::vector<std::string> refs;
    std::vector<Param> params;

    SelectorBuilder(std::string name, std::string type) : name(std::move(name)), selectorType(std::move(type)) {
    }

    void addParam(Param p) {
      params.push_back(p);
    }

    void addRef(std::string ref) {
      refs.push_back(std::move(ref));
    }

    SelectorPtr emitSelector() {
      auto it = selectorRegistry.find(selectorType);
      if (it == selectorRegistry.end()) {
        logError() << "Invalid selector type: " << selectorType << "\n";
        return nullptr;
      }
      // Call factory function
      return it->second(params);
    }

  };

  struct BuilderStack {
    std::vector<SelectorBuilder> stack;

    void beginSelector(std::string name, std::string type) {
      stack.emplace_back(std::move(name), std::move(type));
    }

    void addParam(Param p) {
      getCurrent().addParam(std::move(p));
    }

    void addRef(std::string ref) {
      getCurrent().addRef(std::move(ref));
    }

    bool finalizeSelector(SelectorGraph& graph) {
      assert(!stack.empty() && "No current selector builder");
      auto builder = stack.back();
      stack.pop_back();
      auto selector = builder.emitSelector();
      if (!selector) {
        logError() << "Could not instantiate selector.\n";
        return false;
      }
      if (graph.hasNode(builder.name)) {
        logError() << "Another selector with name " << builder.name <<  " already exists.\n";
        return false;
      }
      auto node = graph.createNode(builder.name, std::move(selector));
      for (auto& ref: builder.refs) {
        node->addInputDependency(ref);
      }
      return true;
    }

    bool empty() const {
      return stack.empty();
    }

  private:
    SelectorBuilder & getCurrent() {
      assert(!stack.empty() && "Tried to access empty decl stack");
      return stack.back();
    }
  };

  BuilderStack builderStack;

  void visitAST(SpecAST &specAst) override {
    visitChildren(specAst);
    graph.setEntryNode(lastDeclName);
  }

  void visitDirective(Directive &directive) override {
    // Skip parameters
    logError() << "The AST still contains a directive, which should have been processed before building the selection pipeline.\n";
    logError() << "Directive: ";
    directive.dump(std::cerr);
    std::cerr << "\n";
  }

  void visitDecl(SelectorDecl &decl) override {
    selectorDeclName = decl.getName();
    if (selectorDeclName.empty()) {
      selectorDeclName = nameGen.next();
    }
    lastDeclName = selectorDeclName;
    visitChildren(decl);
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

  bool hasEncounteredError() {
    return encounteredError;
  }

};

SelectorGraphPtr buildSelectorGraph(SpecAST& ast) {
  auto graph = std::make_unique<SelectorGraph>();
  SelectorEmitter emitter(ast, *graph);
  emitter.visitAST(ast);
  if (emitter.hasEncounteredError())
    return {};
  return graph;
}

}