//
// Created by sebastian on 21.03.22.
//

#ifndef CAPI_XRAYINTERFACE_H
#define CAPI_XRAYINTERFACE_H

#include <string>
#include <map>
#include <vector>

#include "xray/xray_interface.h"

#define XRAY_NEVER_INSTRUMENT __attribute__((xray_never_instrument))


namespace capi {

using XRayHandlerFn = void (*)(int32_t, XRayEntryType);


class ProfilingInterface {
public:
  XRayHandlerFn getHandlerFn();
};


void initXRay();

}

#endif // CAPI_XRAYINTERFACE_H
