//
// Created by sebastian on 01.04.25.
//

#ifndef CAPI_DEMANGLEMD_H
#define CAPI_DEMANGLEMD_H

#include "capi/selection/metadata/TransientMD.h"

namespace capi {

struct FunctionInfo {
  std::string demangledName;
  std::vector<std::string> parameters;
  bool isTrigger{false};
};


struct CaPIMDKey {
  static constexpr const char* key = "capi";
};
using CaPIMD = TransientMD<FunctionInfo, CaPIMDKey>;

//
//class CaPIMD : public metacg::MetaData::Registrar<CaPIMD> {
// public:
//  FunctionInfo info;
//
//  static constexpr const char* key = "capi";
//
//  CaPIMD() = default;
//
//  explicit CaPIMD(const nlohmann::json& j) {
//    // In-memory only
//  }
//
// private:
//  CaPIMD(const CaPIMD& other) : info(other.info) {}
//
// public:
//  nlohmann::json to_json() const final { return {}; }
//
//  virtual const char* getKey() const { return key; }
//
//  void merge(const MetaData& toMerge) final {
//    // Not implemented
//  }
//
//  MetaData* clone() const final { return new CaPIMD(*this); }
//};

}

#endif // CAPI_DEMANGLEMD_H
