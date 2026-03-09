//
// Created by ui72hona on 3/5/26.
//

#include <gtest/gtest.h>

#include "Callgraph.h"
#include "capi/selection/InclusiveMetricAnalysis.h"

#include <initializer_list>

using namespace capi;

CAPI_DEFINE_VERBOSITY(LOG_STATUS)

struct Helper {
    std::unique_ptr<metacg::Callgraph> cg;
    TraversalHelper helper;

    Helper() : cg(std::make_unique<metacg::Callgraph>()), helper(*cg, false){
    }

    CgNode& makeNode(const std::string& name, int count) {
        auto& node = cg->insert(name, {}, false, true);
        node.getOrCreate<cage::NumInstructionsMD>(count);
        return node;
    }

    void makeNodes(std::initializer_list<std::string> nodes, int count) {
        for (auto& node: nodes) {
            makeNode(node, count);
        }
    }

    void addEdges(const std::string caller, std::initializer_list<std::string> callees) {
        for (auto& callee : callees) {
            std::cout << "Adding edge: " << caller << ", " << callee << "\n";
            cg->addEdge(caller, callee);
        }
    }

    bool hasEdge(const std::string& a, const std::string& b) {
        return cg->existsAnyEdge(a, b);
    }

    long getCount(const std::string& node) {
        return cg->getFirstNode(node)->getOrCreate<IICMD>().value;
    }

    bool runAnalysis() {
        InclusiveMetricAnalysis<InstructionCountTraits> analysis;
        return analysis.run(helper);
    }

};


TEST(InclusiveMetricTest, BasicDAG) {
    Helper h;
    h.makeNode("main", 5);
    h.makeNode( "a", 5);
    h.makeNode("b", 5);
    h.makeNode( "c", 5);

    h.addEdges("main", {"a"});
    h.addEdges("a", {"b", "c"});
    h.addEdges("b", {"c"});

    ASSERT_TRUE(h.hasEdge("main", "a"));
    ASSERT_TRUE(h.hasEdge("a", "b"));
    ASSERT_TRUE(h.hasEdge("a", "c"));
    ASSERT_TRUE(h.hasEdge("b", "c"));

    ASSERT_TRUE(h.runAnalysis());

    ASSERT_EQ(h.getCount("main"), 20);
    ASSERT_EQ(h.getCount("a"), 15);
    ASSERT_EQ(h.getCount("b"), 10);
    ASSERT_EQ(h.getCount("c"), 5);
}

TEST(InclusiveMetricTest, OneCycle) {
    Helper h;
    h.makeNodes({"main", "a", "cycle_b", "cycle_c", "d", "e"}, 5);

    h.addEdges("main", {"a", "e"});
    h.addEdges("a", {"cycle_b"});
    h.addEdges("cycle_b", {"cycle_c"});
    h.addEdges("cycle_c", {"cycle_b", "d"});
    h.addEdges("e", {"e"});

    ASSERT_TRUE(h.runAnalysis());

    ASSERT_EQ(h.getCount("main"), 30);
    ASSERT_EQ(h.getCount("a"), 20);
    ASSERT_EQ(h.getCount("cycle_b"), 15);
    ASSERT_EQ(h.getCount("cycle_c"), 15);
    ASSERT_EQ(h.getCount("d"), 5);
    ASSERT_EQ(h.getCount("e"), 5);
}

