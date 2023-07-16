#include <stddef.h>

extern const struct SCOREP_Subsystem SCOREP_Subsystem_Substrates;
extern const struct SCOREP_Subsystem SCOREP_Subsystem_TaskStack;
extern const struct SCOREP_Subsystem SCOREP_Subsystem_MetricService;
extern const struct SCOREP_Subsystem SCOREP_Subsystem_Topologies;
extern const struct SCOREP_Subsystem SCOREP_Subsystem_PlatformTopology;
extern const struct SCOREP_Subsystem SCOREP_Subsystem_CompilerAdapter;
extern const struct SCOREP_Subsystem SCOREP_Subsystem_IoManagement;
extern const struct SCOREP_Subsystem SCOREP_Subsystem_MpiAdapter;

const struct SCOREP_Subsystem* scorep_subsystems[] = {
    &SCOREP_Subsystem_Substrates,
    &SCOREP_Subsystem_TaskStack,
    &SCOREP_Subsystem_MetricService,
    &SCOREP_Subsystem_Topologies,
    &SCOREP_Subsystem_PlatformTopology,
    &SCOREP_Subsystem_CompilerAdapter,
    &SCOREP_Subsystem_IoManagement,
    &SCOREP_Subsystem_MpiAdapter
};

const size_t scorep_number_of_subsystems = sizeof( scorep_subsystems ) /
                                           sizeof( scorep_subsystems[ 0 ] );
