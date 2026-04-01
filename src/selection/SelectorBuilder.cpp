//
// Created by sebastian on 15.03.22.
//

#include "capi/selection/SelectorBuilder.h"
#include "SelectorRegistry.h"
#include "capi/selection/SelectionQueryAST.h"
#include "capi/support/Logging.h"
#include "selectors/BasicSelectors.h"
#include "nlohmann/json.hpp"

namespace capi {

namespace {
std::unordered_map<std::string, SelectorInfo> selectorRegistry;
}

std::optional<SelectorDoc> getSelectorDocumentation(std::string selectorName) {
  auto it = selectorRegistry.find(selectorName);
  if (it == selectorRegistry.end()) {
    return std::nullopt;
  }
  return it->second.doc;
}

RegisterSelector::RegisterSelector(std::string selectorType, SelectorFactoryFn fn, std::optional<SelectorDoc> doc) {
  //std::cout << "Registered selector: " << selectorType << "\n";
  selectorRegistry.emplace(selectorType, SelectorInfo{std::move(fn), std::move(doc)});
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

  QueryAST& ast;
  SelectorGraph& graph;
  bool lastDeclIsEntry;

  bool encounteredError{false};

  std::string pipelineDeclName;
  std::string lastDeclName;

  PipelineNode* lastPipelineEmitted;

  NameGen nameGen;

public:

  SelectorEmitter(QueryAST& ast, SelectorGraph& graph, bool lastDeclIsEntry) : ast(ast), graph(graph), lastDeclIsEntry(lastDeclIsEntry), lastPipelineEmitted(nullptr), nameGen("anon_") {
//    selectorDeclName = "";
    std::vector<SelectorPtr> s;
    s.push_back(std::make_unique<EverythingSelector>());
    graph.createNode("%", std::move(s));
  }

  struct PipelineBuilder {
    std::string name;
    std::vector<std::string> refs;
    std::vector<SelectorPtr> selectors;

//    std::string selectorType;
//    std::vector<Param> params;

    PipelineBuilder(std::string name) : name(std::move(name)) {
    }

    void addRef(const std::string& ref) {
      refs.push_back(ref);
    }

    bool appendSelector(std::string selectorType, const std::vector<Param>& params) {
      auto it = selectorRegistry.find(selectorType);
      if (it == selectorRegistry.end()) {
        logError() << "Invalid selector type: " << selectorType << "\n";
        return false;
      }
      // Call factory function
      selectors.push_back(it->second.fn(params));
      return true;
    }

  };

  struct BuilderStack {
    std::vector<PipelineBuilder> stack;

    std::vector<Param> params;

    void beginPipeline(std::string name) {
      stack.emplace_back(std::move(name));
    }

    void addRef(const std::string& ref) {
      getCurrent().addRef(ref);
    }

    void saveParam(Param p) {
      params.push_back(std::move(p));
    }

    bool emitSelector(std::string selectorType) {
      bool success = getCurrent().appendSelector(std::move(selectorType), params);
      params.clear();
      return success;
    }

    PipelineNode* finalizePipeline(SelectorGraph& graph) {
      assert(!stack.empty() && "No current selector builder");
      auto builder = std::move(stack.back());
      stack.pop_back();
      if (graph.hasNode(builder.name)) {
        logError() << "Another pipeline with name " << builder.name <<  " already exists.\n";
        return nullptr;
      }
      auto node = graph.createNode(builder.name, std::move(builder.selectors));
      for (auto& ref: builder.refs) {
        node->addInputDependency(ref);
      }
      // If this was not a top-level pipeline, add to refs of pipeline on the level below
      if (!stack.empty()) {
        addRef(builder.name);
      }
      return node;
    }

    bool empty() const {
      return stack.empty();
    }

  private:
   PipelineBuilder & getCurrent() {
      assert(!stack.empty() && "Tried to access empty decl stack");
      return stack.back();
    }
  };

  BuilderStack builderStack;

//  PipelineBuilder builder;

  void visitAST(QueryAST&queryAst) override {
    visitChildren(queryAst);
    if (lastDeclIsEntry) {
      graph.addEntryNode(lastDeclName);
    }
  }

  void visitDirective(Directive &directive) override {
    // Skip parameters
    logError() << "The AST still contains a directive, which should have been processed before building the selection pipeline.\n";
    logError() << "Directive: ";
    directive.dump(std::cerr);
    std::cerr << "\n";
  }

  void visitPipelineDecl(PipelineDecl&decl) override {
    pipelineDeclName = decl.getName();
    if (pipelineDeclName.empty()) {
      pipelineDeclName = nameGen.next();
    }
    lastDeclName = pipelineDeclName;
    visitChildren(decl);
    pipelineDeclName = "";
  }

  void visitPipelineExpr(PipelineExpr& pipeline) override {
    // We emit a pipeline node if the pipeline defines selectors, or it is a simple alias for another pipeline.
    if (pipeline.doesDefineSelectors() || pipeline.isAlias()) {
      // Use the name of the current decl, or generate one
      std::string pipelineName = builderStack.empty() ? pipelineDeclName : nameGen.next();;
      builderStack.beginPipeline(pipelineName);
      visitChildren(pipeline);
      lastPipelineEmitted = builderStack.finalizePipeline(graph);
      return;
    }

    // This expression is either a tuple or another nested expression/operation
    visitChildren(pipeline);
  }

  void visitPipelineOp(PipelineOp& op) override {
    if (op.getOperatorType() != OperatorType::ID) {
      std::string pipelineName = builderStack.empty() ? pipelineDeclName : nameGen.next();
      builderStack.beginPipeline(pipelineName);
      visitChild(op, 0);
      if (!lastPipelineEmitted) {
        logError() << "Could not apply operator - there is not input pipeline\n";
        encounteredError = true;
        return;
      }
      auto* operand1 = lastPipelineEmitted;
      visitChild(op, 1);
      if (!lastPipelineEmitted) {
        logError() << "Could not apply operator - there is not input pipeline\n";
        encounteredError = true;
        return;
      }
      auto* operand2 = lastPipelineEmitted;

      switch (op.getOperatorType()) {
        case OperatorType::INTERSECT:
          builderStack.emitSelector("intersect");
          break;
        case OperatorType::DIFF:
          builderStack.emitSelector("subtract");
          break;
        case OperatorType::UNION:
          builderStack.emitSelector("join");
          break;
        default:
          assert(false && "Unhandled operator case");
      }
      builderStack.finalizePipeline(graph);
    } else {
      visitChildren(op);
    }
  }

  void visitSelectorDef(SelectorDef &def) override {
    visitChildren(def);

    bool success = builderStack.emitSelector(def.getType());

    if (!success) {
      logError() << "Could not instantiate selector.\n";
      encounteredError = true;
      return;
    }

  }

  void visitRef(PipelineRef&ref) override {
    builderStack.addRef(ref.getIdentifier());
    visitChildren(ref);
  }

  void visitBoolLiteral(Literal<bool> &l) override {
    auto val = l.getValue();
    builderStack.saveParam(Param::makeBool(val));
  }

  void visitIntLiteral(Literal<int> &l) override {
    auto val = l.getValue();
    builderStack.saveParam(Param::makeInt(val));
  }

  void visitFloatLiteral(Literal<float> &l) override {
    auto val = l.getValue();
    builderStack.saveParam(Param::makeFloat(val));
  }

  void visitStringLiteral(Literal<std::string> &l) override {
    auto val = l.getValue();
    builderStack.saveParam(Param::makeString(val));
  }

  bool hasEncounteredError() {
    return encounteredError;
  }

};

void simplifyGraph(SelectorGraph& graph) {
  bool changed = false;
  do {
    changed = false;
    for (auto& [id, node]: graph.getNodes()) {
      if (node->getSize() == 0 && !graph.isEntryNode(id)) {
        const auto& input = node->getInputDependencies();
        assert(input.size() == 1 && "pipeline with no selector must have exactly one input");
        const auto& replacementId = input.front();
        graph.replaceUses(id, replacementId);
        graph.eraseUnreachable();
        changed = true;
        break;
      }
    }
  } while(changed);
}

SelectorGraphPtr buildSelectorGraph(QueryAST& ast, bool lastDeclIsEntry) {
  auto graph = std::make_unique<SelectorGraph>();
  SelectorEmitter emitter(ast, *graph, lastDeclIsEntry);
  emitter.visitAST(ast);
  if (emitter.hasEncounteredError())
    return {};
  return graph;
}

}
