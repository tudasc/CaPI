//
// Created by sebastian on 04.01.24.
//

#ifndef CAPI_POSTDOM_H
#define CAPI_POSTDOM_H

#include <deque>
#include <unordered_set>
#include <vector>

#include "CallGraph.h"

namespace capi {

template<typename NodeT>
struct PostDomData {
  using NodeSet = std::unordered_set<const PostDomData<NodeT>*>;
  const NodeT* node{nullptr};
  NodeSet postDoms{};
  bool initialized{false};
};

template<typename NodeT>
using PostDomAnalysisResult = std::unordered_map<const NodeT*, PostDomData<NodeT>>;

template<typename NodeT, typename GraphT>
PostDomAnalysisResult<NodeT> computePostDoms(const GraphT& graph, const NodeT& exitNode);

// Adopted from https://stackoverflow.com/questions/25505868/the-intersection-of-multiple-sorted-arrays
template<typename NodeT>
typename PostDomData<NodeT>::NodeSet intersection (const std::vector<const NodeT*> &nodes, const PostDomAnalysisResult<NodeT>& postDomMap) {
  if (nodes.empty())
    return {};

  auto last_intersection = postDomMap.at(nodes[0]).postDoms;

  typename PostDomData<NodeT>::NodeSet curr_intersection;

  for (std::size_t i = 1; i < nodes.size(); ++i) {
    auto& currSet = postDomMap.at(nodes[i]).postDoms;
    std::set_intersection(last_intersection.begin(), last_intersection.end(),
                          currSet.begin(), currSet.end(),
                          std::inserter(curr_intersection, std::end(curr_intersection)));
    std::swap(last_intersection, curr_intersection);
    curr_intersection.clear();
  }
  return last_intersection;
}

//

template<typename NodeT, typename GraphT>
PostDomAnalysisResult<NodeT> computePostDoms(const GraphT& graph, const NodeT& exitNode) {

  if (!graph.getCallees(&exitNode).empty()) {
    // Only leaf nodes allowed
    return {};
  }

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

    auto callees = graph.getCallees(nodeData.node);

    typename PostDomDataT::NodeSet postDomNew = intersection(callees, postDomMap);
    postDomNew.insert(&nodeData);

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
