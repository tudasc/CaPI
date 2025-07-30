//
// Created by sebastian on 09.07.25.
//

#ifndef CAPI_MEASURMENTCONFIGIO_H
#define CAPI_MEASURMENTCONFIGIO_H

#include "capi/selection/MeasurementConfig.h"

#include <memory>

namespace capi {

  bool write(const MeasurementConfig&, const std::string outFile);

  std::unique_ptr<MeasurementConfig> read(const std::string inFile);

}

#endif  // CAPI_MEASURMENTCONFIGIO_H
