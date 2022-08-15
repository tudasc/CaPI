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


struct DirectiveHandler {

  virtual ~DirectiveHandler() = default;

  virtual void consumeParameter(Param p) = 0;

  virtual bool finalize() = 0;

  virtual bool transform(Directive &directive, SpecAST &ast) = 0;
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
    complete = true;
  }

  bool finalize() override {
    return complete && !error;
  }

  bool transform(Directive& directive, SpecAST &ast) override {
    assert((complete && !error) && "Can't transform invalid import directive.");
    auto parent = findParent(ast, directive);
    // Directive must always be direct child of root AST node.
    if (!parent || parent != &ast) {
      logError() << "Unable to determine parent of directive node.\n";
      return false;
    }

    // TODO: Import file and parse AST

    if (filename.empty()) {
      logError() << "Empty import path.\n";
      return false;
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
      return false;
    }

    bool inserted = ast.insertStmt(std::move(subAST), &directive);

    if (!inserted) {
      logError() << "Could not insert the loaded module into the AST.\n";
      return false;
    }

    auto directivePtr = parent->removeChild(&directive);
    if (!directivePtr) {
      logError() << "Could not remove directive node from AST\n";
    }

    return true;
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
    bool valid = handler->finalize();
    if (valid) {
      handler->transform(directive, *ast);
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
    if (!handler) {
      return;
    }
    auto val = l.getValue();
    handler->consumeParameter(Param::makeString(val));
  }

private:
  SpecAST* ast;
  std::unique_ptr<DirectiveHandler> handler;

};

bool preprocessAST(SpecAST &ast) {
  DirectiveProcessor dp;
  dp.visitAST(ast);
  return true;
}

}