//
// Created by sebastian on 15.03.22.
//

#include "capi/selection/SelectorBuilder.h"
#include "SelectorRegistry.h"
#include "capi/selection/SelectionQueryAST.h"
#include "capi/support/Logging.h"
#include "selectors/BasicSelectors.h"

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

  QueryAST& ast;
  SelectorGraph& graph;
  bool lastDeclIsEntry;

  bool encounteredError{false};

  std::string pipelineDeclName;
  std::string lastDeclName;

  NameGen nameGen;

public:

  SelectorEmitter(QueryAST& ast, SelectorGraph& graph, bool lastDeclIsEntry) : ast(ast), graph(graph), lastDeclIsEntry(lastDeclIsEntry), nameGen("anon_") {
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
      selectors.push_back(it->second(params));
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

    bool finalizePipeline(SelectorGraph& graph) {
      assert(!stack.empty() && "No current selector builder");
      auto builder = std::move(stack.back());
      stack.pop_back();
      if (graph.hasNode(builder.name)) {
        logError() << "Another pipeline with name " << builder.name <<  " already exists.\n";
        return false;
      }
      auto node = graph.createNode(builder.name, std::move(builder.selectors));
      for (auto& ref: builder.refs) {
        node->addInputDependency(ref);
      }
      // If this was not a top-level pipeline, add to refs of pipeline on the level below
      if (!stack.empty()) {
        addRef(builder.name);
      }
      return true;
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
    // Use the name of the current decl, or generate one
    std::string pipelineName = builderStack.empty() ? pipelineDeclName : nameGen.next();;
    builderStack.beginPipeline(pipelineName);
    visitChildren(pipeline);
    builderStack.finalizePipeline(graph);
  }

  void visitDef(SelectorDef &def) override {
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

SelectorGraphPtr buildSelectorGraph(QueryAST& ast, bool lastDeclIsEntry) {
  auto graph = std::make_unique<SelectorGraph>();
  SelectorEmitter emitter(ast, *graph, lastDeclIsEntry);
  emitter.visitAST(ast);
  if (emitter.hasEncounteredError())
    return {};
  return graph;
}

}