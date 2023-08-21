//
// Created by sebastian on 09.08.22.
//

#include "Preprocessor.h"

#include <cassert>
#include <map>
#include <functional>
#include <fstream>

#include "SelectorRegistry.h"
#include "SpecParser.h"
#include "InstrumentationHint.h"


namespace capi {

//using DirectiveReplacement = std::pair<Directive*, NodePtr>;



struct DirectiveReplacement {
  Directive* directive{nullptr};
  NodePtr replacement{};
  bool replaceWithChildren{false};
};

struct DirectiveHandler {

  virtual ~DirectiveHandler() = default;

  virtual void consumeParameter(Param p) = 0;

  virtual void consumeRef(const SelectorRef&) = 0;

  virtual bool finalize(InstrumentationHintCollector&) = 0;

  virtual DirectiveReplacement transform(Directive &directive, SpecAST &ast) = 0;
};

using HandlerPtr = std::unique_ptr<DirectiveHandler>;

struct InstrumentHintHandler : public DirectiveHandler {

  InstrumentHintHandler(InstrumentationType type) : type(type) {
  }

  virtual void consumeParameter(Param p) override {
  }

  void consumeRef(const SelectorRef& ref) override {
    this->refName = ref.getIdentifier();
  }

  bool finalize(InstrumentationHintCollector& collector) override {
    if (refName.empty()) {
      return false;
    }
    collector.addHint({type, refName});
    return true;
  }

  DirectiveReplacement transform(Directive& directive, SpecAST &ast) override {
    return {&directive, nullptr, false};
  }

private:
  std::string refName;
  InstrumentationType type;
};

struct ImportHandler : public DirectiveHandler{

  void consumeRef(const SelectorRef&) override {
  }

  void consumeParameter(Param p) override {
    if (error){
      return;
    }
    if(complete) {
      logError() << "Invalid additional parameters in import directive.\n";
      error = true;
      return;
    }
    if (p.kind != Param::STRING) {
      logError() << "Import directive expected string as input, but received " << p.kindNames[p.kind] << "\n";
      error = true;
      return;
    }
    filename = std::get<std::string>(p.val);
    complete = true;
  }

  bool finalize(InstrumentationHintCollector&) override {
    return complete && !error;
  }


  DirectiveReplacement transform(Directive& directive, SpecAST &ast) override {
    assert((complete && !error) && "Can't transform invalid import directive.");
    auto parent = findParent(ast, directive);
    // Directive must always be direct child of root AST node.
    if (!parent || parent != &ast) {
      logError() << "Unable to determine parent of directive node.\n";
      return {};
    }

    if (filename.empty()) {
      logError() << "Empty import path.\n";
      return {};
    }

    std::ifstream in(filename);

    std::string specStr;

    std::string line;
    while (std::getline(in, line)) {
      specStr += line;
    }

    std::ifstream fin(filename);

    SpecParser parser(specStr);
    auto subAST = parser.parse();

    if (!subAST) {
      logError() << "Unable to parse specified module file.\n";
      return {};
    }

    return {&directive, std::move(subAST), true};


  }

private:
  bool error{false};
  bool complete{false};
  std::string filename{""};

};

namespace  {
  std::map<std::string, std::function<HandlerPtr()>> handlerFactoryMap {
    {"import", []() -> HandlerPtr{ return std::make_unique<ImportHandler>();}},
    {"instrument", []() -> HandlerPtr{return std::make_unique<InstrumentHintHandler>(InstrumentationType::ALWAYS_INSTRUMENT);}},
    {"begin_after", []() -> HandlerPtr{return std::make_unique<InstrumentHintHandler>(InstrumentationType::BEGIN_TRIGGER);}}};

  HandlerPtr createHandler(std::string type) {
    auto it = handlerFactoryMap.find(type);
    if (it == handlerFactoryMap.end()) {
      return nullptr;
    }
    return it->second();
  }
}


class DirectiveProcessor : public ASTVisitor, public InstrumentationHintCollector {
public:

  explicit DirectiveProcessor(InstrumentationHints& instHints) : instHints(instHints) {}

  void visitAST(SpecAST &ast) override {
    this->ast = &ast;
    ASTVisitor::visitAST(ast);
  }

  void visitDirective(Directive &directive) override {
    if (handler) {
      logError() << "Cannot consume directive. Another directive handler is still active.\n";
      return;
    }
    handler = createHandler(directive.getName());
    if (!handler) {
      logError() << "Unknown directive '" << directive.getName() << "'. Skipping.\n";
      return;
    }
    visitChildren(directive);
    bool valid = handler->finalize(*this);
    if (valid) {
      auto repment = handler->transform(directive, *ast);
      if (repment.directive) {
        replacements.push_back(std::move(repment));
      }
    }
    handler.release();
  }

  void visitRef(SelectorRef &ref) override {
    if (!handler) {
      return;
    }
    handler->consumeRef(ref);
  }

  void visitDecl(SelectorDecl &decl) override {
    // Decls can be skipped completely
    return;
  }

  void visitBoolLiteral(Literal<bool> &l) override {
    if (!handler) {
      return;
    }
    auto val = l.getValue();
    handler->consumeParameter(Param::makeBool(val));
  }

  void visitIntLiteral(Literal<int> &l) override {
    if (!handler) {
      return;
    }
    auto val = l.getValue();
    handler->consumeParameter(Param::makeInt(val));
  }

  void visitFloatLiteral(Literal<float> &l) override {
    if (!handler) {
      return;
    }
    auto val = l.getValue();
    handler->consumeParameter(Param::makeFloat(val));
  }

  void visitStringLiteral(Literal<std::string> &l) override {
    if (!handler) {
      return;
    }
    auto val = l.getValue();
    handler->consumeParameter(Param::makeString(val));
  }

  void applyReplacements() {

    //auto applyReplacement = []()

    for (auto& r : replacements) {
      // Delete if replacement is null
      if (r.replacement) {
        bool inserted{false};
        ASTNode *insertPoint = r.directive;
        if (r.replaceWithChildren) {
          for (auto &child : r.replacement->getChildren()) {
            auto nextInsertPoint = child.get();
            inserted = ast->insertStmt(std::move(child), insertPoint);
            insertPoint = nextInsertPoint;
          }
        } else {
          inserted = ast->insertStmt(std::move(r.replacement), insertPoint);
        }

        if (!inserted) {
          logError() << "Could not insert the directive replacement.\n";
          return;
        }
      }
      auto directivePtr = ast->removeChild(r.directive);
      if (!directivePtr) {
        logError() << "Could not remove directive node from AST\n";
      }
    }
  }

  void addHint(InstrumentationHint hint) override {
    instHints.push_back(std::move(hint));
  }

private:
  SpecAST* ast;
  std::unique_ptr<DirectiveHandler> handler;

  std::vector<DirectiveReplacement> replacements;
  InstrumentationHints & instHints;

};

bool preprocessAST(SpecAST &ast, InstrumentationHints& instHints) {
  DirectiveProcessor dp(instHints);
  dp.visitAST(ast);

  dp.applyReplacements();

  return true;
}

}