//
// Created by sebastian on 02.04.25.
//

#ifndef CAPI_TRANSIENTMD_H
#define CAPI_TRANSIENTMD_H

#include "metacg/metadata/MetaData.h"

namespace capi {

template <typename DataT, typename KeyT>
class TransientMD : public metacg::MetaData::Registrar<TransientMD<DataT, KeyT>> {
 public:
  static constexpr const char* key = KeyT::key;

  DataT value;

  explicit TransientMD() = default;

  explicit TransientMD(const nlohmann::json& j, metacg::StrToNodeMapping&) {
    // In-memory only
  }

  virtual const char* getKey() const final { return key; }

 private:
  TransientMD(const TransientMD& other) : value(other.value) {}

  nlohmann::json toJson(metacg::NodeToStrMapping&) const final { return {}; }

  void merge(const metacg::MetaData& toMerge, std::optional<metacg::MergeAction>, const metacg::GraphMapping& ) final {}

  std::unique_ptr<metacg::MetaData> clone() const final {
    return std::unique_ptr<TransientMD<DataT, KeyT>>(new TransientMD<DataT, KeyT>(*this));
  }

  void applyMapping(const metacg::GraphMapping&) override {}
};

}

#endif  // CAPI_TRANSIENTMD_H
