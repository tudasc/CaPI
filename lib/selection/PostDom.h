//
// Created by sebastian on 04.01.24.
//

#ifndef CAPI_POSTDOM_H
#define CAPI_POSTDOM_H

#include <deque>
#include <set>
#include <unordered_set>
#include <vector>

#include "CallGraph.h"

namespace capi {

template<typename NodeT>
struct PostDomData {
  using NodeSet = std::unordered_set<const NodeT*>;
  const NodeT* node{nullptr};
  NodeSet postDoms{};
  bool initialized{false};
};

template<typename NodeT>
using PostDomAnalysisResult = std::unordered_map<const NodeT*, PostDomData<NodeT>>;

template<typename NodeT, typename GraphT>
PostDomAnalysisResult<NodeT> computePostDoms(const GraphT& graph, const NodeT& exitNode);

template<typename Container>
Container unordered_intersection(const Container& a, const Container& b) {
//  std::set<typename Container::value_type> orderedA(a.begin(), a.end());
//  std::set<typename Container::value_type> orderedB(b.begin(), b.end());
  Container result;
//  std::set_intersection(orderedA.begin(), orderedA.end(),
//                        orderedB.begin(), orderedB.end(),
//                        std::inserter(result, result.begin()));
  for (auto& item : a) {
    if (std::find(b.begin(), b.end(), item) != b.end()) {
      result.insert(item);
    }
  }
  return result;
}

// Adopted from https://stackoverflow.com/questions/25505868/the-intersection-of-multiple-sorted-arrays
template<typename NodeT>
typename PostDomData<NodeT>::NodeSet intersection (const std::vector<const NodeT*> &nodes, const PostDomAnalysisResult<NodeT>& postDomMap) {
  if (nodes.empty())
    return {};

  auto last_intersection = postDomMap.at(nodes[0]).postDoms;

  typename PostDomData<NodeT>::NodeSet curr_intersection;

  for (std::size_t i = 1; i < nodes.size(); ++i) {
    auto& currSet = postDomMap.at(nodes[i]).postDoms;
    //LOG_STATUS("Intersecting " << dumpNodeSet("a", last_intersection) << " and " << dumpNodeSet("b", currSet) << "\n");

    // Note: std::set_intersection does not work with unordered_set.
    curr_intersection = unordered_intersection(last_intersection, currSet);
//    std::set_intersection(last_intersection.begin(), last_intersection.end(),
//                          currSet.begin(), currSet.end(),
//                          std::inserter(curr_intersection, curr_intersection.begin()));
    //LOG_STATUS("Result of intersection" << dumpNodeSet("", curr_intersection) << "\n");
    std::swap(last_intersection, curr_intersection);
    curr_intersection.clear();
  }
  return last_intersection;
}

//

template<typename NodeT, typename GraphT>
PostDomAnalysisResult<NodeT> computePostDoms(const GraphT& graph, const NodeT& exitNode) {

  using PostDomDataT = PostDomData<NodeT>;

  PostDomAnalysisResult<NodeT> postDomMap{{&exitNode, {&exitNode, {}, false}}};

  std::deque<PostDomDataT*> workQueue{&postDomMap[&exitNode]};

  auto addToQueue = [&workQueue](PostDomDataT* data) {
    if (std::find(workQueue.begin(), workQueue.end(), data) ==
        workQueue.end()) {
      workQueue.push_back(data);
    }
  };

  do {
    auto& nodeData = *workQueue.front();
    workQueue.pop_front();

    std::vector<const NodeT*> initializedCallees{};
    auto callees = graph.getCallees(nodeData.node);
    for (auto& callee : callees) {
      if (postDomMap[callee].initialized) {
        initializedCallees.push_back(callee);
      }
    }

    typename PostDomDataT::NodeSet postDomNew = intersection(initializedCallees, postDomMap);
    postDomNew.insert(nodeData.node);

    //LOG_STATUS("New postDoms " << dumpNodeSet(nodeData.node->getName(), postDomNew) << "\n");

    if (!nodeData.initialized || nodeData.postDoms != postDomNew) {
      nodeData.postDoms = std::move(postDomNew);
      nodeData.initialized = true;

      auto callers = graph.getCallers(nodeData.node);
      for (auto& caller : callers) {
        auto& callerData = postDomMap[caller];
        if (!callerData.node) {
          callerData.node = caller;
        }
        addToQueue(&callerData);
      }

    }
  } while (!workQueue.empty());

  return postDomMap;
}

}

#endif // CAPI_POSTDOM_H
