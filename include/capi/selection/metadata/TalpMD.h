/**
 * File: TalpMD.h
 * License:
 */
#ifndef TALP_MD_H
#define TALP_MD_H

// TODO: Find common place to share this metadata with selective tracing project

#include "metadata/MetaData.h"

#include <algorithm>
#include <numeric>
#include <optional>
#include <string>
#include <vector>

namespace capi {

    struct TalpMetrics {
        /*! Total number of CPUs used by the processes that have used the region */
        unsigned int num_cpus{0};
        /*! Total number of MPI Ranks used by the processes that have used the region */
        unsigned int num_mpi_ranks{0};
        /*! Total number of GPUs used by the processes that have used the region */
        unsigned int num_gpus{0};
        /*! Total number of CPU cycles elapsed in that region during useful time */
        double cycles{0};
        /*! Total number of instructions executed during useful time */
        double instructions{0};
        /*! Number of measurements of this region */
        unsigned long num_measurements{0};
        /*! Number of executed MPI calls combined among all MPI processes */
        unsigned long num_mpi_calls{0};
        /*! Number of encountered OpenMP parallel regions combined among all processes */
        unsigned long num_omp_parallels{0};
        /*! Number of encountered OpenMP tasks combined among all processes */
        unsigned long num_omp_tasks{0};
        /*! Number of executed GPU Runtime calls combined among all processes */
        unsigned long num_gpu_runtime_calls{0};
        /*! Time (in nanoseconds) of the accumulated elapsed time inside the region */
        double elapsed_time{0};
        /*! Time (in nanoseconds) of the accumulated CPU time of useful computation in the application */
        double useful_time{0};
        /*! Efficiency number [0.0 - 1.0] of the impact in the application's parallelization */
        float parallel_efficiency{0};
        /*! Efficiency number of the impact in the MPI parallelization */
        float mpi_parallel_efficiency{0};
        /*! Efficiency lost due to MPI transfer and serialization */
        float mpi_communication_efficiency{0};
        /*! Efficiency of the MPI Load Balance */
        float mpi_load_balance{0};
        /*! Intra-node MPI Load Balance coefficient */
        float mpi_load_balance_in{0};
        /*! Inter-node MPI Load Balance coefficient */
        float mpi_load_balance_out{0};
        /*! Efficiency number of the impact in the OpenMP parallelization */
        float omp_parallel_efficiency{0};
        /*! Efficiency of the OpenMP Load Balance inside parallel regions */
        float omp_load_balance{0};
        /*! Efficiency of the OpenMP scheduling inside parallel regions */
        float omp_scheduling_efficiency{0};
        /*! Efficiency lost due to OpenMP threads outside of parallel regions */
        float omp_serialization_efficiency{0};
        /*! Efficiency of the Host offloading to the Device */
        float device_offload_efficiency{0};
        /*! TBD */
        float gpu_parallel_efficiency{0};
        /*! TBD */
        float gpu_load_balance{0};
        /*! TBD */
        float gpu_communication_efficiency{0};
        /*! TBD */
        float gpu_orchestration_efficiency{0};
    };

    void to_json(nlohmann::json& j, const TalpMetrics& m) {
        j = nlohmann::json{
                {"numCpus", m.num_cpus},
                {"numMpiRanks", m.num_mpi_ranks},
                {"numGpus", m.num_gpus},
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
        m.num_mpi_ranks = j.value("numMpiRanks", 0);
        m.num_gpus = j.value("numGpus", 0);
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

            if (!j.contains("metrics")) {
                metacg::MCGLogger::logWarnUnique("TalpMD entry is missing a metrics field!");
            }

            auto& jMetrics = j["metrics"];
            metrics = jMetrics.get<TalpMetrics>();

            if (j.contains("dynamicallyFiltered")) {
                j.at("dynamicallyFiltered").get_to(dynamicallyFiltered);
            }
        }

        bool wasDynamicallyFiltered() const {
            return dynamicallyFiltered;
        }

        bool hasGPUs() const {
            return metrics.num_gpus > 0;
        }

        void setDynamicallyFiltered(bool filtered) {
            this->dynamicallyFiltered = filtered;
        }

        void setMetrics(TalpMetrics metrics) {
            this->metrics = std::move(metrics);
        }

        const TalpMetrics& getMetrics() const {
            return metrics;
        }

    private:
        TalpMD(const TalpMD& other) : metrics(other.metrics), dynamicallyFiltered(other.dynamicallyFiltered) {}

    public:
        nlohmann::json toJson(metacg::NodeToStrMapping&) const final {
            return nlohmann::json({{"metrics", metrics}, {"dynamicallyFiltered", dynamicallyFiltered}});
        }

        const char* getKey() const final { return key; }

        void merge(const MetaData& toMerge, std::optional<metacg::MergeAction>, const metacg::GraphMapping&) final {
            assert(toMerge.getKey() == getKey() && "Trying to merge TalpMD with meta data of different types");
            metacg::MCGLogger::logWarn(
                    "TalpMD is not meant to be merged, as it is attached to the completed dynamic CG. Keeping MD of original node.");
        }

        std::unique_ptr<MetaData> clone() const final { return std::unique_ptr<TalpMD>(new TalpMD(*this)); }

        virtual void applyMapping(const metacg::GraphMapping&) final {}

        bool dynamicallyFiltered{false};
        TalpMetrics metrics;
    };


}
#endif  // TALP_MD_H