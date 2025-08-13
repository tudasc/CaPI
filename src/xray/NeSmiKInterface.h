//
// Created by sebastian on 23.04.25.
//

#ifndef CAPI_NESMIKINTERFACE_H
#define CAPI_NESMIKINTERFACE_H

namespace capi {

enum class Mode {
  PROFILE, TRACE
};

//class NeSmiKMode {
// public:
//  virtual ~NeSmiKMode() = default;
//  virtual void handleRegionEnter(int id) = 0;
//  virtual void handleRegionExit(int id) = 0;
//};
//
//class ProfilingMode: public NeSmiKMode {
// public:
//  ProfilingMode(bool dynamicFiltering) : dynamicFiltering(dynamicFiltering) {}
//
//  void handleRegionEnter(int id) override;
//
//  void handleRegionExit(int id) override;
// private:
//  bool dynamicFiltering;
//};
//
//class TracingMode: public NeSmiKMode {
// public:
//  void handleRegionEnter(int id) override;
//
//  void handleRegionExit(int id) override;
//};

}

extern "C" {
  void dyncapi_nesmik_init();
  void dyncapi_nesmik_finalize();
}

#endif  // CAPI_NESMIKINTERFACE_H
