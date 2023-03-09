//
// Created by sebastian on 22.03.22.
//

#ifndef CAPI_SCOREPINIT_H
#define CAPI_SCOREPINIT_H

#include "SymbolRetriever.h"
#include "XRayRuntime.h"

#include <cstdint>

extern "C" {
typedef uint32_t SCOREP_LineNo;
struct scorep_compiler_hash_node;

scorep_compiler_hash_node* scorep_compiler_hash_put(uint64_t key, const char* region_name_mangled, const char* region_name_demangled, const char* file_name, SCOREP_LineNo line_no_begin);

void SCOREP_InitMeasurement();

};

namespace capi {
void initScoreP(const XRayFunctionMap& xrayMap);
}

#endif // CAPI_SCOREPINIT_H
