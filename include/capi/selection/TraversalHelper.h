//
// Created by sebastian on 01.04.25.
//

#ifndef CAPI_TRAVERSALHELPER_H
#define CAPI_TRAVERSALHELPER_H

#include "Callgraph.h"
#include "capi/support/IteratorUtils.h"
#include <CgNode.h>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace capi {


/**
 * Traverses the call graph, calling the given visit function on each node.
 * @tparam TraverseFn Function that takes a metacg::CgNode& argument and returns the next
 * nodes to traverse.
 * @tparam VisitFn Function that takes a metacg::CgNode& argument.
 * @param node
 * @param visit
 * @returns The number of visited functions.
 */
template <typename TraverseFn, typename VisitFn>
int traverseCallGraph(const metacg::CgNode &node, TraverseFn &&selectNextNodes,
                      VisitFn &&visit) {
  std::vector<const metacg::CgNode *> workingSet;
  std::unordered_set<const metacg::CgNode *> alreadyVisited;

  workingSet.push_back(&node);

  do {
    auto currentNode = workingSet.back();
    workingSet.pop_back();
    //        std::cout << "Visiting caller " << currentNode->getName() << "\n";
    visit(*currentNode);
    alreadyVisited.insert(currentNode);
    for (auto &nextNode : selectNextNodes(*currentNode)) {
      if (!alreadyVisited.contains(nextNode)
          && std::find(alreadyVisited.begin(), alreadyVisited.end(), nextNode) == alreadyVisited.end()) {
        workingSet.push_back(nextNode);
      }
    }

  } while (!workingSet.empty());

  return alreadyVisited.size();
}

inline bool isNodeDestructor(const metacg::CgNode& node) {
  auto name = node.getFunctionName();
  return name.substr(0, 2) == "_Z" && !(name.compare(name.length() - 4, 4, "D0Ev")
                                                    && name.compare(name.length() - 4, 4, "D1Ev")
                                                    && name.compare(name.length() - 4, 4, "D2Ev"));
}

template<typename T1, typename T2>
void insertIds(T1& container, T2& nodes) {
  for (auto& node : nodes) {
    container.push_back(node->getId());
  }
}

using ConstCgNodePtrSet = std::unordered_set<const metacg::CgNode*>;

struct TraversalHelper;

struct NodeTraversalInfo {
  TraversalHelper* helper{nullptr};
  const metacg::CgNode* node{nullptr};
  ConstCgNodePtrSet callees;
  ConstCgNodePtrSet callers;
  ConstCgNodePtrSet virtualCalls;
  ConstCgNodePtrSet virtualCalledBy;
  std::vector<size_t> overrides;
  std::vector<size_t> overriddenBy;

  bool overridesComputed{false};
  bool overriddenByComputed{false};
  bool callersComputed{false};
  bool calleesComputed{false};
  bool ancestorsComputed{false};
  bool descendantsComputed{false};
  bool isDestructor{false};

  ConstCgNodePtrSet recursiveOverrides;
  ConstCgNodePtrSet recursiveOverriddenBy;
  ConstCgNodePtrSet allCallers;
  ConstCgNodePtrSet allCallees;
  ConstCgNodePtrSet allAncestors;
  ConstCgNodePtrSet allDescendants;

  void compute(const metacg::CgNode& node, TraversalHelper* helper);

  IterRange<decltype(callees.begin())> getCallees() {
    return IterRange(callees.begin(), callees.end());
  }

  IterRange<decltype(callers.begin())> getCallers() {
    return IterRange(callers.begin(), callers.end());
  }

  IterRange<decltype(callees.cbegin())> getCallees() const {
    return IterRange(callees.cbegin(), callees.cend());
  }

  IterRange<decltype(callers.cbegin())> getCallers() const {
    return IterRange(callers.cbegin(), callers.cend());
  }

  IterRange<decltype(overriddenBy.begin())> getOverriddenBy() {
    return {overriddenBy.begin(), overriddenBy.end()};
  }

  IterRange<decltype(overriddenBy.cbegin())> getOverriddenBy() const {
    return {overriddenBy.cbegin(), overriddenBy.cend()};
  }

  IterRange<decltype(overrides.begin())> getOverrides() {
    return {overrides.begin(), overrides.end()};
  }

  IterRange<decltype(overrides.cbegin())> getOverrides() const {
    return {overrides.cbegin(), overrides.cend()};
  }

  IterRange<decltype(recursiveOverrides.begin())> findAllOverrides() {
    updateOverridesCache();
    return {recursiveOverrides.begin(), recursiveOverrides.end()};
  }

  IterRange<decltype(recursiveOverriddenBy.cbegin())> findAllOverriddenBy() {
    updateOverriddenByCache();
    return {recursiveOverriddenBy.cbegin(), recursiveOverriddenBy.cend()};
  }

  IterRange<decltype(allCallers.begin())> findAllCallers() {
    updateAllCallersCache();
    return {allCallers.begin(), allCallers.end()};
  }

  IterRange<decltype(allCallers.begin())> findAllCallees() {
    updateAllCalleesCache();
    return {allCallees.begin(), allCallees.end()};
  }

  IterRange<decltype(allAncestors.begin())> findAllAncestors() {
    updateAllAncestorsCache();
    return {allAncestors.begin(), allAncestors.end()};
  }

  IterRange<decltype(allDescendants.begin())> findAllDescendants() {
    updateAllDescendantsCache();
    return {allDescendants.begin(), allDescendants.end()};
  }

private:
  void updateOverridesCache();

  void updateOverriddenByCache();

  void updateAllCallersCache();

  void updateAllCalleesCache();

  void updateAllAncestorsCache();

  void updateAllDescendantsCache();
};


struct TraversalHelper {
  metacg::Callgraph& cg;
  std::unordered_map<size_t, NodeTraversalInfo> nodeMap;

  bool traverseVirtualDtors;

  TraversalHelper(metacg::Callgraph& cg, bool traverseVirtualDtors) : cg(cg), traverseVirtualDtors(traverseVirtualDtors) {
  }

  NodeTraversalInfo & get(const metacg::CgNode* node) {
    auto& nodeCache = nodeMap[node->getId()];
    if (!nodeCache.node) {
      nodeCache.compute(*node, this);
    }
    return nodeCache;
  }

  NodeTraversalInfo * get(size_t id) {
    auto* node =  cg.getNode(id);
    if (!node) {
      return nullptr;
    }
    return &get(node);
  }

  std::vector<const metacg::CgNode*> findRoots() const {
    std::vector<const metacg::CgNode*> roots;
    for (auto& node : cg.getNodes()) {
      if (cg.getCallers(*node).empty())
        roots.push_back(node.get());
    }
    return roots;
  }

  std::vector<const metacg::CgNode*> findLeaves() const {
    std::vector<const metacg::CgNode*> leaves;
    for (auto& node : cg.getNodes()) {
      if (cg.getCallees(*node).empty())
        leaves.push_back(node.get());
    }
    return leaves;
  }

  std::unordered_map<const metacg::CgNode*, ConstCgNodePtrSet> globalAncestorComputation() {
    // result map
    std::unordered_map<const metacg::CgNode*, ConstCgNodePtrSet> res;

    // Queue of nodes to be processed
    std::vector<const metacg::CgNode*> worklist;

    // When "descending" into a node's parent, this tracks the "call stack" to handle
    // cycles in the call graph
    std::vector<const metacg::CgNode*> stack;

    // Temporary vector holding all parents of a node that have not been processed yet
    std::vector<const metacg::CgNode*> missingParents;

    // Temporary vector holding the ancestors of all parents that have already been processed
    std::vector<ConstCgNodePtrSet*> parentAncestorsCollection;

    // Put all leaves into the worklist
    for (auto node : findLeaves()) {
      worklist.push_back(node);
    }

    while (!worklist.empty()) {
      const metacg::CgNode* node = worklist.back();
      worklist.pop_back();
      
      // Skip if this node was already processed
      if (res.contains(node)) {
        continue;
      }
      
      // Fresh temporary vectors
      missingParents.clear();
      parentAncestorsCollection.clear();
      
      NodeTraversalInfo& nti = this->get(node);
      auto parents = nti.findAllCallers();

      // iterate over all parents to popolate `missingParents` and `parentAncestorsCollection`
      for (const metacg::CgNode* parent : parents) {
        auto parent_res = res.find(parent);

        if (parent_res == res.end()) {
          // parent has not been processed yet
          if (std::find(stack.begin(), stack.end(), parent) == stack.end()) {
            // if the parent is not in the stack, put it into `missingParents`
            missingParents.push_back(parent);
          }
        } else {
          // parent has already been processed -> save its ancestors
          parentAncestorsCollection.push_back(&(parent_res->second));
        }
      }

      // check whether all parents have already been processed
      if (!missingParents.empty()) {
        // there are still parents which have not yet been processed

        // push this node on the stack to prevent cycles
        stack.push_back(node);

        // re-visit this node after all parents have been visited
        // (thus push it onto the worklist first)
        worklist.push_back(node);

        // push all missing parents onto the worklist
        for (const metacg::CgNode* parent : missingParents) {
          worklist.push_back(parent);
        }
      } else {
        // all parents have already been processed \o/

        // remove this node from the stack if it is on top
        if (!stack.empty() && stack.back() == node) {
          stack.pop_back();
        }

        // collect this node's ancestors (i.e., its parents and its parent's ancestors)
        ConstCgNodePtrSet ancestors;
        for (const metacg::CgNode* parent : parents) {
          ancestors.insert(parent);
        }
        for (ConstCgNodePtrSet* parentAncestors : parentAncestorsCollection) {
          for (const metacg::CgNode* parentAncestor : *parentAncestors) {
            ancestors.insert(parentAncestor);
          }
        }

        // save into result map
        res[node] = ancestors;
      }
    }

    return res;
  }

  bool shouldTraverseVirtualDtors() const {
    return traverseVirtualDtors;
  }

};



}

#endif // CAPI_TRAVERSALHELPER_H
