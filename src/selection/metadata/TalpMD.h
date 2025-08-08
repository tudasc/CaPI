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
  /*! Number of executed MPI calls combined among all MPI processes */
  unsigned long num_mpi_calls;
  /*! Number of encountered OpenMP parallel regions combined among all processes */
  unsigned long num_omp_parallels;
  /*! Number of encountered OpenMP tasks combined among all processes */
  unsigned long num_omp_tasks;
  /*! Time (in nanoseconds) of the accumulated elapsed time inside the region */
  double elapsed_time;
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
};

void to_json(nlohmann::json& j, const TalpMetrics& m) {
  j = nlohmann::json{
      {"numCpus", m.num_cpus},
      {"cycles", m.cycles},
      {"instructions", m.instructions},
      {"numMpiCalls", m.num_mpi_calls},
      {"numOmpParallels", m.num_omp_parallels},
      {"numOmpTasks", m.num_omp_tasks},
      {"elapsedTime", m.elapsed_time},
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
  };
}

void from_json(const nlohmann::json& j, TalpMetrics& m) {
  j.at("numCpus").get_to(m.num_cpus);
  j.at("cycles").get_to(m.cycles);
  j.at("instructions").get_to(m.instructions);
  j.at("numMpiCalls").get_to(m.num_mpi_calls);
  j.at("numOmpParallels").get_to(m.num_omp_parallels);
  j.at("numOmpTasks").get_to(m.num_omp_tasks);
  j.at("elapsedTime").get_to(m.elapsed_time);
  j.at("parallelEfficiency").get_to(m.parallel_efficiency);
  j.at("mpiParallelEfficiency").get_to(m.mpi_parallel_efficiency);
  j.at("mpiCommunicationEfficiency").get_to(m.mpi_communication_efficiency);
  j.at("mpiLoadBalance").get_to(m.mpi_load_balance);
  j.at("mpiLoadBalanceIn").get_to(m.mpi_load_balance_in);
  j.at("mpiLoadBalanceOut").get_to(m.mpi_load_balance_out);
  j.at("ompParallelEfficiency").get_to(m.omp_parallel_efficiency);
  j.at("ompLoadBalance").get_to(m.omp_load_balance);
  j.at("ompSchedulingEfficiency").get_to(m.omp_scheduling_efficiency);
  j.at("ompSerializationEfficiency").get_to(m.omp_serialization_efficiency);
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
    // Iterate over all entries in list. Each entry contains metrics for a given
    // call path.
    for (auto it = j.begin(); it != j.end(); ++it) {
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
  TalpMD(const TalpMD& other) : pathMetrics(other.pathMetrics) {}

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
    return j;
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
};

}

#endif  // TALP_MD_H
