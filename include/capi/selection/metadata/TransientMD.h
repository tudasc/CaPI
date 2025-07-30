//
// Created by sebastian on 02.04.25.
//

#ifndef CAPI_TRANSIENTMD_H
#define CAPI_TRANSIENTMD_H

#include "metadata/MetaData.h"

namespace capi {

template <typename DataT, typename KeyT>
class TransientMD : public metacg::MetaData::Registrar<TransientMD<DataT, KeyT>> {
 public:
  static constexpr const char* key = KeyT::key;

  DataT value;

  explicit TransientMD() = default;

  explicit TransientMD(const nlohmann::json& j) {
    // In-memory only
  }

  virtual const char* getKey() const { return key; }

 private:
  TransientMD(const TransientMD& other) : value(other.value) {}

  nlohmann::json to_json() const final { return {}; }

  void merge(const metacg::MetaData& toMerge) final {}

  metacg::MetaData* clone() const final { return new TransientMD<DataT, KeyT>(*this); }
};

}

#endif  // CAPI_TRANSIENTMD_H
