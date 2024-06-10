# CaPI: Compiler-assisted Performance Instrumentation

CaPI is a selective code instrumentation tool, designed for streamlining the performance analysis workflow of large-scale parallel applications.

CaPI selects functions for instrumentation according to the needs of the analyst, based on a static call graph of the target application,.
This creates instrumentation configurations (ICs) that capture relevant parts of the code, while keeping the runtime overhead low.
![capi_overview.png](capi_overview.png)

It consists of two major components:
- A selection tool, that creates ICs tailored to the target code and measurement objective.
- A runtime library, enabling runtime-adaptable binary instrumentation based on [LLVM XRay](https://llvm.org/docs/XRay.html). 

CaPI currently supports the following measurement APIs:
- GNU interface: compatible with GCC's `-finstrument-functions`
- TALP (part of the DLB library): Parallel performance metrics of MPI regions
- Score-P: Instrumentation-based profiling and tracing
- Extrae: MPI-based tracing

This project is currently in a pre-release state, frequent changes to the code and build config can be expected.

## Requirements

- CMake >=3.15
- LLVM >=10
- ScoreP 7 (optional)
- DLB 3.3 (optional, other versions may work)
- Extrae 3.8.3 (optional, other versions may work)
- LLVM-Lit (testing only)

## Build
CaPI is built as follows ([Ninja](https://github.com/ninja-build/ninja) is not required and can be substituted with `make`).
```
mkdir build && cd build
cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=$(which clang) -DCMAKE_CXX_COMPILER=$(which clang++) -DDLB_DIR=$(which dlb)/../.. -DSCOREP_DIR=$(which scorep)/../.. .. 
ninja
```

CMake Options
- `ENABLE_TALP=ON/OFF`: Enable/Disable support for TALP. Default is `ON`.
  - `DLB_DIR`: Path to DLB installation.
- `ENABLE_SCOREP=ON/OFF`: Enable/Disable support for Score-P. Default is `ON`.
  - `SCOREP_DIR`: Path to Score-P installation.
- `ENABLE_EXTRAE=ON/OFF`: Enable/Disable support for Extrae. Default is `ON`.
- `ENABLE_XRAY=ON/OFF`: Enable/Disable support for runtime-adaptable instrumentation with LLVM XRay. Default is `ON`.
- `ENABLE_TESTING=ON/OFF`: Enable/Disable testing. Requires MetaCG and LLVM-Lit.
  - Set `metacg_DIR` to MetaCG installation directory.

## Container
The easiest way to try out CaPI is to install the [apptainer](https://apptainer.org/) provided in the `container` directory.
The container provides installations of CaPI and all dependencies. 
To build it in sandbox mode, run the following command: 
```
apptainer build --sandbox --fakeroot capi container/capi.def
```
Afterwards, you may open a shell into the sandbox as follow:
```
apptainer shell --writable --fakeroot --cleanenv capi
```
Refer to the apptainer documentation for further options.

## Examples
To verify your build, you may test out the instrumentation of proxy applications LULESH and AMG in the `example` folder (located in your current build directory).
For details, refer to `CAPI_README` in the `lulesh` folder.

## Instrumentation Selection

CaPI relies on [MetaCG](https://github.com/tudasc/MetaCG) for its whole-program call graph analysis.

In order to apply CaPI, you first need to install MetaCG and run it on your target application.
See the MetaCG README for instructions.

You can then run CaPI to generate the instrumentation configuration (IC).

The IC is determined by passing the functions in the CG through a composable pipeline of individual selectors.
Each of these selectors produces an output set that is in turn consumed by other selectors.

### Options
This is an overview of the current command line interface.
- `-h`   Print a list of options.
- `-i <query>`   Parse the selection query from the given string.
- `-f <file>`      Use a selection query file.
- `-o <file>`      The output IC file.
- `-v <verbosity>`     Set verbosity level (0-3, default is 2). Passing `-v` without argument sets it to 3.
- `--write-dot <file>`  Write a dotfile of the selected call-graph subset.
- `--replace-inlined <binary>`  Replaces inlined functions with parents. Requires passing the executable.
- `--output-format <output_format>`  Set the file format. Options are `scorep` (default), `json` and `simple`
- `--debug`  Enable debugging mode.
- `--print-scc-stats`  Prints information about the strongly connected components (SCCs) of this call graph.
- `--traverse-virtual-dtors` Enable traversal of virtual destructors (may lead to over-approximation of destructor inheritance).

### Selection Query DSL

CaPI defines a DSL for the user-defined instrumentation selection.
The selection query is passed in as string with the `-i` flag:

```
capi -i '<selection_query>' callgraph.ipcg
```
Alternatively, `-f <file>` instructs CaPI to load the query from the given file.

The query consists of a sequence of selector definitions, which can be named or anonymous.
Each definition takes a list of parameters.
Available parameter types are strings (enclosed in double quotes), booleans (true/false),  integers and floating point numbers.
In addition, most selectors expect another selector definition as input.
These can be either in-place definitions or references to other named selectors, marked with `%`.
The selector `%%` is pre-defined and refers to an instance of the `EverythingSelector`, which selects every function in the CG.

The last definition in the sequence is used as the entry point for the selection pipeline.

For example, the following selector, named `mpi`, finds all functions starting with `MPI_`.
```
mpi = by_name("MPI_.*", %%)
```

This can be used to find all functions that are on a callpath to a MPI call:

```
mpi          = by_name("MPI_.*", %%)
mpi_callpath = on_call_path_to(%mpi)
```

To reduce overhead, it is typically sensible to exclude functions that are marked as `inline`.
Adding this to the previous spec, the result might look like this:

```
mpi          = by_name("MPI_.*", %%)
mpi_callpath = on_call_path_to(%mpi)
final        = subtract(%mpi_callpath, inline_specified(%%))
```
or shortened:
```
subtract(onCallPathTo(by_name("MPI_.*", %%)),inline_specified(%%))
```

Recently, support for loading pre-defined selection modules via the `!import` statement was added.
This allows to build and re-use selectors that are useful across multiple applications.
For example, the `mpi_callpath` selector from the previous example could be moved to a separate file:
```
!include "mpi.capi"
subtract(%mpi_callpath, inlineSpecified(%%))
```

List of available selectors:

| Name                                                               | Parameters          | Selector inputs | Example                                                    | Explanation                                                                                                    |
|--------------------------------------------------------------------|---------------------|-----------------|------------------------------------------------------------|----------------------------------------------------------------------------------------------------------------|
| by_name                                                            | regex string        | 1               | `by_name("foo.*", %%)`                                     | Selects functions with names starting with "foo".                                                              |
| by_path                                                            | regex string        | 1               | `byPath("foo/.*", %%)`                                     | Selects functions contained in directory "foo".                                                                |
| inline_specified                                                   | -                   | 1               | `inline_specified(%%)`                                     | Selects functions marked as `inline`.                                                                          |
| on_call_path_to                                                    | -                   | 1               | `on_call_path_to(by_name("foo", %%))`                      | Selects functions in the call chain to function "foo".                                                         |
| on_call_path_from                                                  | -                   | 1               | `on_call_path_from(by_name("foo", %%))`                    | Selects functions in the call chain from function "foo".                                                       |
| in_system_header                                                   | -                   | 1               | `in_system_header(%%)`                                     | Selects functions defined in system headers.                                                                   |
| contains_unresolved_calls                                          | -                   | 1               | `contains_unresolved_calls(%%)`                            | Selects functions containing calls to unknown target functions.                                                |
| join                                                               | -                   | 2               | `join(%A, %B)`                                             | Union of the two input sets.                                                                                   |
| intersect                                                          | -                   | 2               | `intersect(%A, %B)`                                        | Intersection of the two input sets.                                                                            |
| subtract                                                           | -                   | 2               | `subtract(%A, %B)`                                         | Complement of the two input sets.                                                                              |
| coarse                                                             | -                   | 1 or 2          | `coarse(%A, %B)`                                           | Filter out functions that have a single caller and callee, unless they are included in B.                      |
| min_call_depth                                                     | comp. operator, threshold | 1               | `min_call_depth("<=", 3, %A)`                              | Selects functions that are at most 3 calls away from a root node.                                              |
| flops                                                              | comp. operator, threshold | 1               | `flops(">=", 10, %A)`                                      | Selects functions with at least 10 floating point operations.                                                  |
| loop_depth                                                         | comp. operator, threshold | 1               | `loop_depth("=", 2, %A)`                                   | Selects functions containing loop nests of depth 2.                                                            |
| common_caller<br/>common_caller_distinct<br/>common_caller_partial | heuristic parameter | 2               | `common_caller(1, by_name("foo", %%), by_name("bar", %%))` | Common caller selection with max. LCA-Dist 1 (details [here](#common-caller-selection-for-trace-augmentation)) |

### Common caller selection for trace augmentation
The `common_caller` selectors are specialized heuristics for augmenting MPI based traces. 
To instrument a region in the trace, the surrounding MPI calls X and Y are determined.
Passing the name of the direct callers of X and Y to the `common_caller` query, CaPI selects relevant calls path leading to these calls.
Details will be made available in an upcoming publication.

### Inline compensation
LLVM-XRay currently does not support the instrumentation of inlined functions.
Since the MetaCG call graph is based on the source code, the information whether a function is inlined by the compiler is not directly available to CaPI.
As a result, the IC may contain inlined functions that cannot be instrumented.
The `--replace-inlined <executable_binary>` option was added to compensate this issue.
It detects which functions in the IC are not available in the binary and replaces them with direct callers.

## Instrumentation
The IC generated by CaPI is used to direct the instrumentation of the target application.
Static and dynamic instrumentation methods are supported.
However, due to their flexibility the dynamic instrumentation workflow using LLVM-XRay is prefered, since it allows for rapid iterative adjustments of the selection, 
without requiring the program to be rebuilt.

### Static Instrumentation with CaPI plugin for LLVM (deprecated)
You can use the provided compiler wrappers `clang-inst`/`clang-inst++` to build and instrument program.
Before building, set the environment variable `CAPI_FILTER_FILE` to the name of the generated IC file.
Please note that the compiler wrapper will not automatically link any measurement library.
You will need to pass the corresponding build flags yourself.

### Static Instrumentation with Score-P
Passing `--output-format scorep` to CaPI generates a filter file compatible with Score-P.
This enables directly instrumenting with the Score-P instrumenter.
To do this, simply build with `scorep-g++` and set `SCOREP_WRAPPER_INSTRUMENTER_FLAGS="--instrument-filter=<filter-file>"`.
To enable measuring functions in shared libraries, use the [Score-P Symbol Injector](https://github.com/sebastiankreutzer/scorep-symbol-injector) library.


### Dynamic Instrumentation with LLVM XRay
CaPI now provides a runtime library compatible with [LLVM XRay](https://llvm.org/docs/XRay.html).
Instead of using a statically instrumented build for each IC, this enables dynamic instrumentation during program initialization.
With XRay, only one build is required and ICs can be changed without recompilation.

You can toggle this feature by setting `ENABLE_XRAY=ON` on.
This will generate compiler wrappers in the `scripts` subdirectory of your current build:
- `capi-cxx` for compiling and instrumenting C++ code.
- `capi-mpicxx` for compiling and instrumenting C++ that uses MPI.
These wrappers are fully compatible with the `clang++` frontend and can thus be used as direct replacements.
`hybrid` versions have been added for compatibility with C code. They compile with `clang` and link with `clang++`.
- 
There are currently four different tool interfaces implemented in the following CaPI runtime libraries:
- `libcapixray_gnu.a`: Compatible with `-finstrument-functions`. Calls `__cyg_profile_func_enter` on enter and ``__cyg_profile_func_exit` on exit.
- `libcapixray_scorep.a`: Compatible with the GNU interface of Score-P.
- `libcapixray_talp.a`: Interface for the TALP tool.
- `libcapixray_extrae.a`: Interface for the Extrae tool.

The tool interface is selected in the wrapper by passing `--capi-interface=<gnu/scorep/talp/extrae>`.

To instrument the program at program start, set the environment variable `CAPI_FILTERING_FILE=<ic_file>`.

As an alternative to the wrappers, it is also possible to pass the required flags manually.
When building the target application, you will need to use the Clang compiler and pass the flag `-fxray-instrument`.
XRay uses a pre-filtering mechanism to exclude very small functions. If you want to be able to potentially instrument all functions, you need to pass `-fxray-instruction-threshold=1` as well.
You will then need to link the XRay-compatible CaPI runtime library into your executable by adding the following:
`-Wl,--whole-archive <capi_build_dir>/lib/xray/libcapixray_<capi_interface>.a -Wl,--no-whole-archive`, along with the required LLVM dependencies given by `llvm-config --libfiles xray symbolize --link-static --system-libs`.

**Important note**: The upstream version of LLVM does currently not support XRay instrumentation of shared libraries.
If you need this feature, you can use [this fork of LLVM](https://github.com/sebastiankreutzer/llvm-project-xray-dso) (use branch `xray_dso_main` for the latest version).
The feature is enabled by passing the additional flag `-fxray-enable-shared` when building your application.
We suggest building LLVM with the following CMake flags: `cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/path/to/install/llvm/13.0.1-xray-dso -DLLVM_ENABLE_PROJECTS="clang;compiler-rt;libcxx;libcxxabi;openmp" -DLLVM_TARGETS_TO_BUILD=X86 -DLLVM_INSTALL_UTILS=ON ../llvm/`
Note that on some cluster systems, the CGG toolchain needs to be set explicitly via the `GCC_INSTALL_PREFIX` CMake option.

<!---
## Ongoing development
We are currently evaluating a new call-graph analysis method that runs at link-time.
A LLVM linker plugin embeds the call-graph into the binary.
This allows the CaPI runtime to load the call-graph dynamically and to merge call-graphs of shared libraries on the fly.
This work is currently in development and will be made public in the near future.
-->

## Publications
Kreutzer, S., Iwainsky, C., Lehr, JP., Bischof, C. (2022). Compiler-Assisted Instrumentation Selection for Large-Scale C++ Codes. In: Anzt, H., Bienz, A., Luszczek, P., Baboulin, M. (eds) High Performance Computing. ISC High Performance 2022 International Workshops. ISC High Performance 2022. Lecture Notes in Computer Science, vol 13387. Springer, Cham. https://doi.org/10.1007/978-3-031-23220-6_1

S. Kreutzer, C. Iwainsky, M. Garcia-Gasulla, V. Lopez and C. Bischof, "Runtime-Adaptable Selective Performance Instrumentation," 2023 IEEE International Parallel and Distributed Processing Symposium Workshops (IPDPSW), St. Petersburg, FL, USA, 2023, pp. 423-432, doi: 10.1109/IPDPSW59300.2023.00073.

