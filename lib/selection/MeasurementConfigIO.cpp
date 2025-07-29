//
// Created by sebastian on 09.07.25.
//

#include <fstream>
#include <memory>
#include <nlohmann/json.hpp>

#include "MeasurementConfig.h"
#include "support/Logging.h"

namespace capi {

bool write(const MeasurementConfig& config, const std::string outFile) {
  std::ofstream out(outFile);
  if (!out.is_open()) {
    logError() <<"Failed to open output file: " << outFile << "\n";
    return false;
  }
  nlohmann::json jConfig = config;
  nlohmann::json j{{"selection", jConfig}, {"_format", {{"name", "measurement_config"}, {"version", "1.0"}}}};
  out << j.dump(4);  // pretty print with 4-space indentation
  return true;
}

std::unique_ptr<MeasurementConfig> read(const std::string inFile) {
  std::ifstream in(inFile);
  if (!in.is_open()) {
    throw std::runtime_error("Failed to open input file: " + inFile);
  }

  nlohmann::json j;
  in >> j;

  auto& jFormat = j.at("_format");
  if (jFormat.at("version") != "1.0") {
    logError() << "Only supporting measurement config file format 1.0\n";
    return {};
  }

  auto config = std::make_unique<MeasurementConfig>();
  j.get_to(*config);
  return config;
}

}  // namespace capi