#include "capi/selection/DriverUtils.h"
#include "capi/selection/SelectorBuilder.h"

namespace capi {
    LogLevel verbosity{static_cast<LogLevel>(0)}; // define the global verbosity for this executable
}

int main() {

    auto docs_json = capi::exportSelectorDoc();

    const std::string header = "| Name  | Parameters | Selector inputs | Example | Explanation |\n|-------|------------|-----------------|---------|-------------|\n";
    std::cout << header;
    for (const auto& doc : docs_json) {
        std::cout << "|" << doc["name"] << "|" << doc["parameterTypes"] << "|" <<  doc["examples"] << "|" << doc["explanation"] << "|\n";
    }
    return 0;
}
