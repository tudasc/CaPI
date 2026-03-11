/**
 * File: TalpMD.h
 * License:
 */
#ifndef TALP_MD_H
#define TALP_MD_H

namespace capi {

// TODO: Find common place to share this metadata with selective tracing project

#include "metadata/MetaData.h"

#include <numeric>
#include <optional>
#include <vector>

struct TalpMetrics {
  /*! Total number of CPUs used by the processes that have used the region */
  unsigned int num_cpus;
  /*! Total number of CPU cycles elapsed in that region during useful time */
  double cycles;
  /*! Total number of instructions executed during useful time */
  double instructions;
  /*! Number of measurements of this region */
  unsigned long num_measurements;
  /*! Number of executed MPI calls combined among all MPI processes */
  unsigned long num_mpi_calls;
  /*! Number of encountered OpenMP parallel regions combined among all processes */
  unsigned long num_omp_parallels;
  /*! Number of encountered OpenMP tasks combined among all processes */
  unsigned long num_omp_tasks;
  /*! Number of executed GPU Runtime calls combined among all processes */
  unsigned long num_gpu_runtime_calls;
  /*! Time (in nanoseconds) of the accumulated elapsed time inside the region */
  double elapsed_time;
  /*! Time (in nanoseconds) of the accumulated CPU time of useful computation in the application */
  double useful_time;
  /*! Efficiency number [0.0 - 1.0] of the impact in the application's parallelization */
  float parallel_efficiency;
  /*! Efficiency number of the impact in the MPI parallelization */
  float mpi_parallel_efficiency;
  /*! Efficiency lost due to MPI transfer and serialization */
  float mpi_communication_efficiency;
  /*! Efficiency of the MPI Load Balance */
  float mpi_load_balance;
  /*! Intra-node MPI Load Balance coefficient */
  float mpi_load_balance_in;
  /*! Inter-node MPI Load Balance coefficient */
  float mpi_load_balance_out;
  /*! Efficiency number of the impact in the OpenMP parallelization */
  float omp_parallel_efficiency;
  /*! Efficiency of the OpenMP Load Balance inside parallel regions */
  float omp_load_balance;
  /*! Efficiency of the OpenMP scheduling inside parallel regions */
  float omp_scheduling_efficiency;
  /*! Efficiency lost due to OpenMP threads outside of parallel regions */
  float omp_serialization_efficiency;
  /*! Efficiency of the Host offloading to the Device */
  float device_offload_efficiency;
  /*! TBD */
  float gpu_parallel_efficiency;
  /*! TBD */
  float gpu_load_balance;
  /*! TBD */
  float gpu_communication_efficiency;
  /*! TBD */
  float gpu_orchestration_efficiency;
};

void to_json(nlohmann::json& j, const TalpMetrics& m) {
  j = nlohmann::json{
      {"numCpus", m.num_cpus},
      {"cycles", m.cycles},
      {"instructions", m.instructions},
      {"numMeasurements", m.num_measurements},
      {"numMpiCalls", m.num_mpi_calls},
      {"numOmpParallels", m.num_omp_parallels},
      {"numOmpTasks", m.num_omp_tasks},
      {"numGpuRuntimeCalls", m.num_gpu_runtime_calls},
      {"elapsedTime", m.elapsed_time},
      {"usefulTime", m.useful_time},
      {"parallelEfficiency", m.parallel_efficiency},
      {"mpiParallelEfficiency", m.mpi_parallel_efficiency},
      {"mpiCommunicationEfficiency", m.mpi_communication_efficiency},
      {"mpiLoadBalance", m.mpi_load_balance},
      {"mpiLoadBalanceIn", m.mpi_load_balance_in},
      {"mpiLoadBalanceOut", m.mpi_load_balance_out},
      {"ompParallelEfficiency", m.omp_parallel_efficiency},
      {"ompLoadBalance", m.omp_load_balance},
      {"ompSchedulingEfficiency", m.omp_scheduling_efficiency},
      {"ompSerializationEfficiency", m.omp_serialization_efficiency},
      {"deviceOffloadEfficiency", m.device_offload_efficiency},
      {"gpuParallelEfficiency", m.gpu_parallel_efficiency},
      {"gpuLoadBalance", m.gpu_load_balance},
      {"gpuCommunicationEfficiency", m.gpu_communication_efficiency},
      {"gpuOrchestrationEfficiency", m.gpu_orchestration_efficiency},
  };
}

void from_json(const nlohmann::json& j, TalpMetrics& m) {
  m.num_cpus = j.value("numCpus", 0);
  m.cycles = j.value("cycles", 0.0);
  m.instructions = j.value("instructions", 0.0);
  m.num_measurements = j.value("numMeasurements", 0u);
  m.num_mpi_calls = j.value("numMpiCalls", 0u);
  m.num_omp_parallels = j.value("numOmpParallels", 0u);
  m.num_omp_tasks = j.value("numOmpTasks",0u);
  m.num_gpu_runtime_calls = j.value("numGpuRuntimeCalls", 0u);
  m.elapsed_time = j.value("elapsedTime", 0.0f);
  m.useful_time = j.value("usefulTime", 0.0f);
  m.parallel_efficiency = j.value("parallelEfficiency", 0.0f);
  m.mpi_parallel_efficiency = j.value("mpiParallelEfficiency", 0.0f);
  m.mpi_communication_efficiency = j.value("mpiCommunicationEfficiency", 0.0f);
  m.mpi_load_balance = j.value("mpiLoadBalance", 0.0f);
  m.mpi_load_balance_in = j.value("mpiLoadBalanceIn", 0.0f);
  m.mpi_load_balance_out = j.value("mpiLoadBalanceOut", 0.0f);
  m.omp_parallel_efficiency = j.value("ompParallelEfficiency", 0.0f);
  m.omp_load_balance = j.value("ompLoadBalance", 0.0f);
  m.omp_scheduling_efficiency = j.value("ompSchedulingEfficiency", 0.0f);
  m.omp_serialization_efficiency = j.value("ompSerializationEfficiency", 0.0f);
  m.device_offload_efficiency = j.value("deviceOffloadEfficiency", 0.0f);
  m.gpu_parallel_efficiency = j.value("gpuParallelEfficiency", 0.0f);
  m.gpu_load_balance = j.value("gpuLoadBalance", 0.0f);
  m.gpu_communication_efficiency = j.value("gpuCommunicationEfficiency", 0.0f);
  m.gpu_orchestration_efficiency = j.value("gpuOrchestrationEfficiency", 0.0f);
}

struct TalpPathMetrics {
  TalpPathMetrics(std::vector<std::string> path, TalpMetrics metrics)
      : callPath(std::move(path)), metrics(std::move(metrics)) {}

  std::vector<std::string> callPath;
  TalpMetrics metrics;
};

class TalpMD : public metacg::MetaData::Registrar<TalpMD> {
 public:
  static constexpr const char* key = "talp";
  TalpMD() = default;

  explicit TalpMD(const nlohmann::json& j, metacg::StrToNodeMapping&) {
    metacg::MCGLogger::logInfoUnique("Reading TalpMD from JSON");
    if (j.is_null()) {
      metacg::MCGLogger::logWarnUnique("Could not retrieve meta data for TalpMD");
      return;
    }
    auto& jPathMetricsArray = j.contains("path_metrics") ? j.at("path_metrics") : j;

    // Iterate over all entries in list. Each entry contains metrics for a given
    // call path.
    for (auto it = jPathMetricsArray.begin(); it != jPathMetricsArray.end(); ++it) {
      auto& jPathMetrics = it.value();
      if (!jPathMetrics.contains("path")) {
        metacg::MCGLogger::logWarnUnique("TalpMD entry is missing a path field!");
        continue;
      }
      if (!jPathMetrics.contains("metrics")) {
        metacg::MCGLogger::logWarnUnique("TalpMD entry is missing a metrics field!");
        continue;
      }

      auto& jPath = jPathMetrics["path"];
      std::vector<std::string> callPath = jPath.get<std::vector<std::string>>();

      auto& jMetrics = jPathMetrics["metrics"];

      TalpMetrics metrics = jMetrics.get<TalpMetrics>();
      addMetrics(std::move(callPath), std::move(metrics));
    }
    if (j.contains("dynamicallyFiltered")) {
      j.at("dynamicallyFiltered").get_to(dynamicallyFiltered);
    }
  }

  bool wasDynamicallyFiltered() const {
    return dynamicallyFiltered;
  }

  void setDynamicallyFiltered(bool filtered) {
    this->dynamicallyFiltered = filtered;
  }

  void addMetrics(std::vector<std::string> callPath, TalpMetrics metrics) {
    pathMetrics.emplace_back(std::move(callPath), std::move(metrics));
  }

  const TalpMetrics* findMetrics(std::vector<std::string>& callPath) const {
    // One could probably optimize this lookup
    std::vector<unsigned> searchList(pathMetrics.size());
    std::iota(searchList.begin(), searchList.end(), 0);
    unsigned depth = 0;
    for (auto& regionName : callPath) {
      if (searchList.empty()) {
        return nullptr;
      }
      searchList.erase(std::remove_if(searchList.begin(), searchList.end(),
                                      [this, &regionName, depth](unsigned i) {
                                        return pathMetrics[i].callPath[depth] != regionName;
                                      }),
                       searchList.end());
      depth++;
    }
    if (searchList.size() > 1) {
      metacg::MCGLogger::logWarnUnique("Multiple metric entries for path!");
    }
    return &pathMetrics[searchList.front()].metrics;
  }

 private:
  TalpMD(const TalpMD& other) : pathMetrics(other.pathMetrics), dynamicallyFiltered(other.dynamicallyFiltered) {}

 public:
  nlohmann::json toJson(metacg::NodeToStrMapping&) const final {
    nlohmann::json j = nlohmann::json::array();
    for (auto& pathMetricsEntry : pathMetrics) {
      nlohmann::json jPathMetricsEntry;
      nlohmann::json jPath = pathMetricsEntry.callPath;
      jPathMetricsEntry["path"] = jPath;
      nlohmann::json jMetrics = pathMetricsEntry.metrics;
      jPathMetricsEntry["metrics"] = jMetrics;
      j.push_back(jPathMetricsEntry);
    }
    return nlohmann::json({{"path_metrics", j}, {"dynamicallyFiltered", dynamicallyFiltered}});
  }

  const char* getKey() const final { return key; }

  void merge(const MetaData& toMerge, std::optional<metacg::MergeAction>, const metacg::GraphMapping&) final {
    assert(toMerge.getKey() == getKey() && "Trying to merge TalpMD with meta data of different types");
    metacg::MCGLogger::logWarn(
        "TalpMD is not meant to be merged, as it is attached to the completed static CG. Keeping MD of original node.");
  }

  std::unique_ptr<MetaData> clone() const final { return std::unique_ptr<TalpMD>(new TalpMD(*this)); }

  virtual void applyMapping(const metacg::GraphMapping&) final {}

 private:
  std::vector<TalpPathMetrics> pathMetrics;
  bool dynamicallyFiltered{false};
};

}

#endif  // TALP_MD_H
