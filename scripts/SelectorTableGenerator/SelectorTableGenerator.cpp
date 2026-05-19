#include "capi/selection/DriverUtils.h"
#include <vector>

namespace capi {
LogLevel verbosity{static_cast<LogLevel>(0)};
}
std::string joinJsonArray(const nlohmann::json& arr) {
  std::string result;

  for (size_t i = 0; i < arr.size(); ++i) {
    if (i > 0)
      result += ", ";

    result += arr[i].get<std::string>();
  }

  return result;
}

void generateTableMD() {
  auto docs_json = capi::exportSelectorDoc();

  // Separate DEFAULT and TALP selectors
  std::vector<nlohmann::json> default_selectors;
  std::vector<nlohmann::json> talp_selectors;

  for (const auto& doc : docs_json) {
    int type_val = doc["type"];
    if (type_val == 1) {
      talp_selectors.push_back(doc);
    } else {
      default_selectors.push_back(doc);
    }
  }

  // Generate DEFAULT selectors table
  std::cout << "### DEFAULT Selectors\n\n";
  std::cout << "| Name | Parameters | Selector inputs | Example | Explanation |\n";
  std::cout << "|------|------------|-----------------|---------|-------------|\n";
  for (const auto& doc : default_selectors) {
    std::cout << "|" << doc["name"].get<std::string>() << "|" << joinJsonArray(doc["parameterLabels"]) << "|"
              << doc["numInputs"].get<std::string>() << "|" << doc["examples"].get<std::string>() << "|"
              << doc["explanation"].get<std::string>() << "|\n";
  }

  // Generate TALP selectors table
  std::cout << "\n### TALP Selectors\n\n";
  std::cout << "| Name | Parameters | Selector inputs | Example | Explanation |\n";
  std::cout << "|------|------------|-----------------|---------|-------------|\n";
  for (const auto& doc : talp_selectors) {
    std::cout << "|" << doc["name"].get<std::string>() << "|" << joinJsonArray(doc["parameterLabels"]) << "|"
              << doc["numInputs"].get<std::string>() << "|" << doc["examples"].get<std::string>() << "|"
              << doc["explanation"].get<std::string>() << "|\n";
  }
}

int main() {
  generateTableMD();
  return 0;
}
