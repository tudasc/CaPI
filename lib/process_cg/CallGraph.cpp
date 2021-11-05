//
// Created by sebastian on 28.10.21.
//

#include "CallGraph.h"

#include <cassert>
#include <iostream>


CGNode& CallGraph::getOrCreate(const std::string& name) {
    auto it = nodeMap.find(name);
    if (it == nodeMap.end()) {
        auto idx = nodes.size();
        nodes.emplace_back(std::make_unique<CGNode>(name));
        nodeMap[name] = idx;
        return *nodes.back();
    }
    assert(it->second < nodes.size() && "Node map is corrupted");
    return *nodes[it->second];
}

CGNode* CallGraph::get(const std::string& name) {
    auto it = nodeMap.find(name);
    if (it == nodeMap.end())
        return nullptr;
    assert(it->second < nodes.size() && "Node map is corrupted");
    return &*nodes[it->second];
}

void CallGraph::addCallee(CGNode &parent, CGNode &child) {
    parent.addCallee(&child);
    child.addCaller(&parent);
}

void CallGraph::removeCallee(CGNode &parent, CGNode &child) {
    parent.removeCallee(&child);
    child.removeCaller(&parent);
}

std::unique_ptr<CallGraph> createCG(FInfoMap& fInfoMap) {
    auto cg = std::make_unique<CallGraph>();
    for (auto& [name, info] : fInfoMap) {
        auto& node = cg->getOrCreate(name);
        node.setFunctionInfo(info);
        std::for_each(info.callees.begin(), info.callees.end(), [&cg, &node](const std::string callee) {
            auto& calleeNode = cg->getOrCreate(callee);
           cg->addCallee(node, calleeNode);
        });
    }
    return cg;
}