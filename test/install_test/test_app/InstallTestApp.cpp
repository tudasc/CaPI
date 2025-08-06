//
// Created by sebastian on 06.08.25.
//
#include <iostream>

#include "capi/support/Logging.h"
#include "capi/selection/DriverUtils.h"
#include "capi/selection/Demangle.h"

#include "io/MCGReader.h"

using namespace capi;

CAPI_DEFINE_VERBOSITY(LOG_STATUS)

int main(int argc, char** argv) {
  std::cout << "Running install integration test for CaPI library\n";

  // Input graph from one of the selection integration tests
  std::string inputGraph = R"json("{"_CG":{"_Z1ai":{"callees":["_Z1bi"],"callers":["main"],"doesOverride":false,"hasBody":true,"isVirtual":false,"meta":{"codeStatistics":{"numVars":2},"estimateCallCount":{"calls":{"_Z1bi":[[1.0,"@3967-4009"]]},"codeRegions":{"@3967-4009":{"functions":["_Z1bi"],"parent":"","parentCalls":100.0}}},"fileProperties":{"origin":"/home/sebastian/git/capi/test/selection/artificial/01_basic.cpp","systemInclude":false},"globalLoopDepth":3,"inlineInfo":{"isTemplate":false,"likelyInline":false,"markedAlwaysInline":false,"markedInline":false},"loopCallDepth":{"_Z1bi":1},"loopDepth":1,"mallocCollector":[],"numConditionalBranches":1,"numOperations":{"numberOfControlFlowOps":3,"numberOfFloatOps":0,"numberOfIntOps":4,"numberOfMemoryAccesses":0},"numStatements":2},"overriddenBy":[],"overrides":[]},"_Z1bi":{"callees":["_Z1cii"],"callers":["_Z1ai"],"doesOverride":false,"hasBody":true,"isVirtual":false,"meta":{"codeStatistics":{"numVars":3},"estimateCallCount":{"calls":{"_Z1cii":[[1.0,"@3891-3940"]]},"codeRegions":{"@3857-3944":{"functions":[],"parent":"","parentCalls":100.0},"@3891-3940":{"functions":["_Z1cii"],"parent":"@3857-3944","parentCalls":100.0}}},"fileProperties":{"origin":"/home/sebastian/git/capi/test/selection/artificial/01_basic.cpp","systemInclude":false},"globalLoopDepth":2,"inlineInfo":{"isTemplate":false,"likelyInline":false,"markedAlwaysInline":false,"markedInline":false},"loopCallDepth":{"_Z1cii":2},"loopDepth":2,"mallocCollector":[],"numConditionalBranches":2,"numOperations":{"numberOfControlFlowOps":5,"numberOfFloatOps":0,"numberOfIntOps":8,"numberOfMemoryAccesses":0},"numStatements":3},"overriddenBy":[],"overrides":[]},"_Z1cii":{"callees":[],"callers":["_Z1bi","main"],"doesOverride":false,"hasBody":true,"isVirtual":false,"meta":{"codeStatistics":{"numVars":3},"estimateCallCount":{"calls":{},"codeRegions":{}},"fileProperties":{"origin":"/home/sebastian/git/capi/test/selection/artificial/01_basic.cpp","systemInclude":false},"globalLoopDepth":0,"inlineInfo":{"isTemplate":false,"likelyInline":false,"markedAlwaysInline":false,"markedInline":false},"loopCallDepth":{},"loopDepth":0,"mallocCollector":[],"numConditionalBranches":0,"numOperations":{"numberOfControlFlowOps":1,"numberOfFloatOps":8,"numberOfIntOps":0,"numberOfMemoryAccesses":0},"numStatements":2},"overriddenBy":[],"overrides":[]},"_Z1dv":{"callees":[],"callers":[],"doesOverride":false,"hasBody":true,"isVirtual":false,"meta":{"codeStatistics":{"numVars":0},"estimateCallCount":{"calls":{},"codeRegions":{}},"fileProperties":{"origin":"/home/sebastian/git/capi/test/selection/artificial/01_basic.cpp","systemInclude":false},"globalLoopDepth":0,"inlineInfo":{"isTemplate":false,"likelyInline":false,"markedAlwaysInline":false,"markedInline":false},"loopCallDepth":{},"loopDepth":0,"mallocCollector":[],"numConditionalBranches":0,"numOperations":{"numberOfControlFlowOps":0,"numberOfFloatOps":0,"numberOfIntOps":0,"numberOfMemoryAccesses":0},"numStatements":0},"overriddenBy":[],"overrides":[]},"main":{"callees":["_Z1ai","_Z1cii"],"callers":[],"doesOverride":false,"hasBody":true,"isVirtual":false,"meta":{"codeStatistics":{"numVars":2},"estimateCallCount":{"calls":{"_Z1ai":[[1.0,""]],"_Z1cii":[[1.0,""]]},"codeRegions":{}},"fileProperties":{"origin":"/home/sebastian/git/capi/test/selection/artificial/01_basic.cpp","systemInclude":false},"globalLoopDepth":3,"inlineInfo":{"isTemplate":false,"likelyInline":false,"markedAlwaysInline":false,"markedInline":false},"loopCallDepth":{"_Z1ai":0,"_Z1cii":0},"loopDepth":0,"mallocCollector":[],"numConditionalBranches":0,"numOperations":{"numberOfControlFlowOps":3,"numberOfFloatOps":0,"numberOfIntOps":0,"numberOfMemoryAccesses":0},"numStatements":3},"overriddenBy":[],"overrides":[]}},"_MetaCG":{"generator":{"name":"CGCollector","sha":"7e409592c27de3d2bad0810ed8868b51546c9595","version":"0.7"},"version":"2.0"}}")json";
  auto inputJson = "{\"_CG\":{\"_Z1ai\":{\"callees\":[\"_Z1bi\"],\"callers\":[\"main\"],\"doesOverride\":false,\"hasBody\":true,\"isVirtual\":false,\"meta\":{\"codeStatistics\":{\"numVars\":2},\"estimateCallCount\":{\"calls\":{\"_Z1bi\":[[1.0,\"@3967-4009\"]]},\"codeRegions\":{\"@3967-4009\":{\"functions\":[\"_Z1bi\"],\"parent\":\"\",\"parentCalls\":100.0}}},\"fileProperties\":{\"origin\":\"/home/sebastian/git/capi/test/selection/artificial/01_basic.cpp\",\"systemInclude\":false},\"globalLoopDepth\":3,\"inlineInfo\":{\"isTemplate\":false,\"likelyInline\":false,\"markedAlwaysInline\":false,\"markedInline\":false},\"loopCallDepth\":{\"_Z1bi\":1},\"loopDepth\":1,\"mallocCollector\":[],\"numConditionalBranches\":1,\"numOperations\":{\"numberOfControlFlowOps\":3,\"numberOfFloatOps\":0,\"numberOfIntOps\":4,\"numberOfMemoryAccesses\":0},\"numStatements\":2},\"overriddenBy\":[],\"overrides\":[]},\"_Z1bi\":{\"callees\":[\"_Z1cii\"],\"callers\":[\"_Z1ai\"],\"doesOverride\":false,\"hasBody\":true,\"isVirtual\":false,\"meta\":{\"codeStatistics\":{\"numVars\":3},\"estimateCallCount\":{\"calls\":{\"_Z1cii\":[[1.0,\"@3891-3940\"]]},\"codeRegions\":{\"@3857-3944\":{\"functions\":[],\"parent\":\"\",\"parentCalls\":100.0},\"@3891-3940\":{\"functions\":[\"_Z1cii\"],\"parent\":\"@3857-3944\",\"parentCalls\":100.0}}},\"fileProperties\":{\"origin\":\"/home/sebastian/git/capi/test/selection/artificial/01_basic.cpp\",\"systemInclude\":false},\"globalLoopDepth\":2,\"inlineInfo\":{\"isTemplate\":false,\"likelyInline\":false,\"markedAlwaysInline\":false,\"markedInline\":false},\"loopCallDepth\":{\"_Z1cii\":2},\"loopDepth\":2,\"mallocCollector\":[],\"numConditionalBranches\":2,\"numOperations\":{\"numberOfControlFlowOps\":5,\"numberOfFloatOps\":0,\"numberOfIntOps\":8,\"numberOfMemoryAccesses\":0},\"numStatements\":3},\"overriddenBy\":[],\"overrides\":[]},\"_Z1cii\":{\"callees\":[],\"callers\":[\"_Z1bi\",\"main\"],\"doesOverride\":false,\"hasBody\":true,\"isVirtual\":false,\"meta\":{\"codeStatistics\":{\"numVars\":3},\"estimateCallCount\":{\"calls\":{},\"codeRegions\":{}},\"fileProperties\":{\"origin\":\"/home/sebastian/git/capi/test/selection/artificial/01_basic.cpp\",\"systemInclude\":false},\"globalLoopDepth\":0,\"inlineInfo\":{\"isTemplate\":false,\"likelyInline\":false,\"markedAlwaysInline\":false,\"markedInline\":false},\"loopCallDepth\":{},\"loopDepth\":0,\"mallocCollector\":[],\"numConditionalBranches\":0,\"numOperations\":{\"numberOfControlFlowOps\":1,\"numberOfFloatOps\":8,\"numberOfIntOps\":0,\"numberOfMemoryAccesses\":0},\"numStatements\":2},\"overriddenBy\":[],\"overrides\":[]},\"_Z1dv\":{\"callees\":[],\"callers\":[],\"doesOverride\":false,\"hasBody\":true,\"isVirtual\":false,\"meta\":{\"codeStatistics\":{\"numVars\":0},\"estimateCallCount\":{\"calls\":{},\"codeRegions\":{}},\"fileProperties\":{\"origin\":\"/home/sebastian/git/capi/test/selection/artificial/01_basic.cpp\",\"systemInclude\":false},\"globalLoopDepth\":0,\"inlineInfo\":{\"isTemplate\":false,\"likelyInline\":false,\"markedAlwaysInline\":false,\"markedInline\":false},\"loopCallDepth\":{},\"loopDepth\":0,\"mallocCollector\":[],\"numConditionalBranches\":0,\"numOperations\":{\"numberOfControlFlowOps\":0,\"numberOfFloatOps\":0,\"numberOfIntOps\":0,\"numberOfMemoryAccesses\":0},\"numStatements\":0},\"overriddenBy\":[],\"overrides\":[]},\"main\":{\"callees\":[\"_Z1ai\",\"_Z1cii\"],\"callers\":[],\"doesOverride\":false,\"hasBody\":true,\"isVirtual\":false,\"meta\":{\"codeStatistics\":{\"numVars\":2},\"estimateCallCount\":{\"calls\":{\"_Z1ai\":[[1.0,\"\"]],\"_Z1cii\":[[1.0,\"\"]]},\"codeRegions\":{}},\"fileProperties\":{\"origin\":\"/home/sebastian/git/capi/test/selection/artificial/01_basic.cpp\",\"systemInclude\":false},\"globalLoopDepth\":3,\"inlineInfo\":{\"isTemplate\":false,\"likelyInline\":false,\"markedAlwaysInline\":false,\"markedInline\":false},\"loopCallDepth\":{\"_Z1ai\":0,\"_Z1cii\":0},\"loopDepth\":0,\"mallocCollector\":[],\"numConditionalBranches\":0,\"numOperations\":{\"numberOfControlFlowOps\":3,\"numberOfFloatOps\":0,\"numberOfIntOps\":0,\"numberOfMemoryAccesses\":0},\"numStatements\":3},\"overriddenBy\":[],\"overrides\":[]}},\"_MetaCG\":{\"generator\":{\"name\":\"CGCollector\",\"sha\":\"7e409592c27de3d2bad0810ed8868b51546c9595\",\"version\":\"0.7\"},\"version\":\"2.0\"}}"_json;

  metacg::io::JsonSource readerSrc(inputJson);
  auto reader = metacg::io::createReader(readerSrc);
  if (!reader) {
    std::cerr << "Unable to create reader for input file\n";
    return EXIT_FAILURE;
  }

  auto cg = reader->read();
  if (!cg) {
    std::cerr << "Failed to read call graph\n";
    return EXIT_FAILURE;
  }

  std::cout << "Demangling...\n";
  // Attach demangled names as metadata
  demangleNames(*cg);

  std::cout << "Loaded CG with " << cg->size() << " nodes\n";

  // Create selection runner
  SelectionRunner runner(*cg, false);

  // Simple query
  std::string queryStr = "by_name(\".*\", %%)";

  auto resultOrErr = runner.runQuery(queryStr, false, false);

  if (!resultOrErr) {
    std::cerr << "Selection query failed with error: " << resultOrErr.error() << "\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
