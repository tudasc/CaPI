
#ifndef CAPI_NESMIKMERGER_H
#define CAPI_NESMIKMERGER_H

#include "Callgraph.h"

#include <memory>

namespace capi {
    std::unique_ptr<metacg::Callgraph> buildDynamicGraphAndAttachMetrics(metacg::Callgraph* staticGraph,
                                                                         const nlohmann::json& talp_json,
                                                                         const nlohmann::json& nesmik_json,
                                                                         const std::unordered_set<std::string>& dynamic_filter_list);


}

#endif //CAPI_NESMIKMERGER_H
