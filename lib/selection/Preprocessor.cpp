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

  virtual bool finalize() = 0;

  virtual DirectiveReplacement transform(Directive &directive, SpecAST &ast) = 0;
};

using HandlerPtr = std::unique_ptr<DirectiveHandler>;

struct ImportHandler : public DirectiveHandler{

  virtual void consumeParameter(Param p) override {
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
    std::cout << "Reading import string: " << filename << "\n";
    complete = true;
  }

  bool finalize() override {
    return complete && !error;
  }


  DirectiveReplacement transform(Directive& directive, SpecAST &ast) override {
    assert((complete && !error) && "Can't transform invalid import directive.");
    logError() << "Transforming directive\n";
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
  std::map<std::string, std::function<HandlerPtr()>> handlerFactoryMap {{"import", []() -> HandlerPtr{
        return std::make_unique<ImportHandler>();
      }}};

  HandlerPtr createHandler(std::string type) {
    auto it = handlerFactoryMap.find(type);
    if (it == handlerFactoryMap.end()) {
      return nullptr;
    }
    return it->second();
  }
}


class DirectiveProcessor : public ASTVisitor {
public:
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
    logError() << "Directive visiting done\n";
    bool valid = handler->finalize();
    logError() << "Directive finalize done\n";

    if (valid) {
      auto repment = handler->transform(directive, *ast);
      if (repment.directive && repment.replacement) {
        replacements.push_back(std::move(repment));
      }
    }
    handler.release();
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
    std::cout << "Actually visiting string literal\n";
    if (!handler) {
      return;
    }
    auto val = l.getValue();
    std::cout << "Val is " << val << "\n";
    handler->consumeParameter(Param::makeString(val));
  }

  void applyReplacements() {

    //auto applyReplacement = []()

    for (auto& r : replacements) {
      bool inserted{false};
      ASTNode* insertPoint = r.directive;
      if (r.replaceWithChildren) {
        for (auto& child : r.replacement->getChildren()) {
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
      auto directivePtr = ast->removeChild(r.directive);
      if (!directivePtr) {
        logError() << "Could not remove directive node from AST\n";
      }
    }
  }

private:
  SpecAST* ast;
  std::unique_ptr<DirectiveHandler> handler;

  std::vector<DirectiveReplacement> replacements;

};

bool preprocessAST(SpecAST &ast) {
  DirectiveProcessor dp;
  dp.visitAST(ast);

  dp.applyReplacements();

  return true;
}

}