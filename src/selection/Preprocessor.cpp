//
// Created by sebastian on 09.08.22.
//

#include "capi/selection/Preprocessor.h"

#include <cassert>
#include <map>
#include <functional>
#include <fstream>

#include "SelectorRegistry.h"
#include "capi/selection/InstrumentationAction.h"
#include "capi/selection/QueryParser.h"

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

  virtual bool finalize(InstrumentationActionCollector&) = 0;

  virtual DirectiveReplacement transform(Directive &directive, QueryAST&ast) = 0;
};

using HandlerPtr = std::unique_ptr<DirectiveHandler>;

struct InstrumentActionHandler : public DirectiveHandler {

  InstrumentActionHandler(InstrumentationType type) : type(type) {
  }

  virtual void consumeParameter(Param p) override {
    if (p.kind != Param::STRING) {
      logError() << "Instrument directive expected range string as input, but received " << p.kindNames[p.kind] << "\n";
      return;
    }
    auto paramStr = std::get<std::string>(p.val);
    // Remove whitespaces
    paramStr.erase(std::remove_if(paramStr.begin(), paramStr.end(),
                           [](unsigned char c){ return std::isspace(c); }),
                   paramStr.end());

    auto colonPos = paramStr.find(':');
    if (colonPos == std::string::npos) {
      activeInvocations[paramStr] = {};
      return;
    }
    std::string lvl = paramStr.substr(0, colonPos);
    std::string rangeStr = paramStr.substr(colonPos + 1);

    Invocations invocations;
    bool isRange = false;
    unsigned rangeStart;
    std::string idxStr;
    for (int pos = 0; pos < rangeStr.size(); pos++) {
      char c = rangeStr[pos];
      bool isLastChar = (pos == rangeStr.size() -1);
      if (std::isdigit(c)) {
        idxStr += c;
        if (!isLastChar) {
          continue;
        }
      }
      unsigned idx = std::stoi(idxStr);
      idxStr = "";
      if (isLastChar || c == ',') {
        if (isRange) {
          invocations.emplace_back(rangeStart, idx);
          isRange = false;
        } else {
          invocations.emplace_back(idx, idx);
        }
      } else if (c == '-') {
        rangeStart = idx;
        isRange = true;
      } else {
          logError() << "Invalid character '" << c << "' in invocation range. Valid characters are digits, ',', and '-'\n";
          return;
      }
    }
    this->activeInvocations[lvl] = invocations;
  }

  void consumeRef(const SelectorRef& ref) override {
    this->refName = ref.getIdentifier();
  }

  bool finalize(InstrumentationActionCollector& collector) override {
    if (refName.empty()) {
      return false;
    }
    collector.addAction({type, refName, activeInvocations});
    return true;
  }

  DirectiveReplacement transform(Directive& directive, QueryAST&ast) override {
    return {&directive, nullptr, false};
  }

private:
  std::string refName;
  InstrumentationType type;
  MappedInvocations activeInvocations;
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

  bool finalize(InstrumentationActionCollector&) override {
    return complete && !error;
  }


  DirectiveReplacement transform(Directive& directive, QueryAST&ast) override {
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

    std::string queryStr;

    std::string line;
    while (std::getline(in, line)) {
      queryStr += line;
    }

    std::ifstream fin(filename);

    QueryParser parser(queryStr);
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
    {"instrument", []() -> HandlerPtr{return std::make_unique<InstrumentActionHandler>(InstrumentationType::ALWAYS_INSTRUMENT);}},
    {"begin_at", []() -> HandlerPtr{return std::make_unique<InstrumentActionHandler>(InstrumentationType::BEGIN_TRIGGER);}},
    {"end_at", []() -> HandlerPtr{return std::make_unique<InstrumentActionHandler>(InstrumentationType::END_TRIGGER);}}};

  HandlerPtr createHandler(std::string type) {
    auto it = handlerFactoryMap.find(type);
    if (it == handlerFactoryMap.end()) {
      return nullptr;
    }
    return it->second();
  }
}


class DirectiveProcessor : public ASTVisitor, public InstrumentationActionCollector {
public:

  explicit DirectiveProcessor(InstrumentationActions& instActions) : instActions(instActions) {}

  void visitAST(QueryAST&ast) override {
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
    handler.reset();
  }

  void visitRef(SelectorRef &ref) override {
    if (!handler) {
      return;
    }
    handler->consumeRef(ref);
  }

  void visitDecl(SelectorDecl &decl) override {
    // Decls can be skipped completely
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

  void addAction(InstrumentationAction hint) override {
    instActions.push_back(std::move(hint));
  }

private:
 QueryAST* ast;
  std::unique_ptr<DirectiveHandler> handler;

  std::vector<DirectiveReplacement> replacements;
  InstrumentationActions & instActions;

};

bool preprocessAST(QueryAST&ast, InstrumentationActions& instActions) {
  DirectiveProcessor dp(instActions);
  dp.visitAST(ast);

  dp.applyReplacements();

  return true;
}

}