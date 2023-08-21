# CaPI: Compiler-assisted Performance Instrumentation

Selective instrumentation tool based on InstRO.

This project is currently in a pre-release state, frequent changes to the code and build config can be expected.

## Requirements

- LLVM 10
- ScoreP 7
- CMake >=3.15
- DLB 3.3 (optional)

If you use Score-P for profiling and want to measure shared libraries, we suggest using the [Score-P Symbol Injector](https://github.com/sebastiankreutzer/scorep-symbol-injector) library.

## Build
You can build CaPI as follows ([Ninja](https://github.com/ninja-build/ninja) is not required, you can use make instead).
```
mkdir build && cd build
cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -DDLB_DIR=$(which dlb)/../.. -DSCOREP_PATH=$(which scorep)/../.. .. 
ninja
```
Options
- `ENABLE_TALP=ON/OFF`: Enable/Disable support for TALP interface. Default is `ON`.
- `ENABLE_XRAY=ON/OFF`: Enable/Disable support for runtime-adaptable instrumentation with LLVM XRay. Default is `ON`.

## Usage

CaPI relies on [MetaCG](https://github.com/tudasc/MetaCG) for its whole-program call graph analysis.

In order to apply CaPI, you first need to install MetaCG and run it on your target application.
See the MetaCG README for instructions.

You can then run CaPI to generate the instrumentation configuration (IC).

The IC is determined by passing the functions in the CG through a composable pipeline of individual selectors.
Each of these selectors produces an output set that is in turn consumed by other selectors.

Currently, there are options to either use a pre-defined selection method or to pass in a custom selection specification as a string.

### Predefined Selection Pipelines

The `MPI` preset can be used to track all functions that are on a call-path to MPI.
It can be applied as follows:
```
capi -p MPI callgraph.ipcg
```
This will create the IC in `callgraph.filt`.

### Selection Specification DSL

CaPI defines a DSL for the user-defined instrumentation selection.
This specifications is passed in as string along with the `-i` flag:

```
capi -i '<selection_spec>' callgraph.ipcg
```

The specification consists of a sequence of selector definitions, which can be named or anonymous.
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
mpi_callpath = onCallPathTo(%mpi)
```

To reduce overhead, it is typically sensible to exclude functions that are marked as `inline`.
Adding this to the previous spec, the result might look like this:

```
mpi          = by_name("MPI_.*", %%)
mpi_callpath = onCallPathTo(%mpi)
final        = subtract(%mpi_callpath, inlineSpecified(%%))
```
or shortened:
```
subtract(onCallPathTo(by_name("MPI_.*", %%)),inlineSpecified(%%))
```

Recently, support for loading pre-defined selection modules via the `!import` statement was added.
This allows to build and re-use selectors that are useful across multiple applications.
For example, the `mpi_callpath` selector from the previous example could be moved to a separate file:
```
!include "mpi.capi"
subtract(%mpi_callpath, inlineSpecified(%%))
```

List of available selectors:

| Name                    | Parameters                | Selector inputs | Example                            | Explanation                                                                               |
|-------------------------|---------------------------|-----------------|------------------------------------|-------------------------------------------------------------------------------------------|
| by_name                  | regex string              | 1               | `by_name("foo.*", %%)`              | Selects functions with names starting with "foo".                                         |
| byPath                  | regex string              | 1               | `byPath("foo/.*", %%)`             | Selects functions contained in directory "foo".                                           |
| inlineSpecified         | -                         | 1               | `inlineSpecified(%%)`              | Selects functions marked as `inline`.                                                     |
| onCallPathTo            | -                         | 1               | `onCallPathTo(by_name("foo", %%))`  | Selects functions in the call chain to function "foo".                                    |
| onCallPathFrom          | -                         | 1               | `onCallPathFrom(by_name("foo", %%))` | Selects functions in the call chain from function "foo".                                  |
| inSystemHeader          | -                         | 1               | `inSystemHeader(%%)`               | Selects functions defined in system headers.                                              |
| containsUnresolvedCalls | -                         | 1               | `containsUnresolvedCalls(%%)`      | Selects functions containing calls to unknown target functions.                           |
| join                    | -                         | 2               | `join(%A, %B)`                     | Union of the two input sets.                                                              |
| intersect               | -                         | 2               | `intersect(%A, %B)`                | Intersection of the two input sets.                                                       |
| subtract                | -                         | 2               | `subtract(%A, %B)`                 | Complement of the two input sets.                                                         |
| coarse                  | -                         | 1 or 2          | `coarse(%A, %B)`                   | Filter out functions that have a single caller and callee, unless they are included in B. |
| minCallDepth            | comp. operator, threshold | 1               | `minCallDepth("<=", 3, %A)`        | Selects functions that are at most 3 calls away from a root node.                         |
| flops                   | comp. operator, threshold | 1               | `flops(">=", 10, %A)`              | Selects functions with at least 10 floating point operations.                             |
| loopDepth               | comp. operator, threshold | 1               | `loopDepth("=", 2, %A)`            | Selects functions containing loop nests of depth 2.                                       |

### Instrumentation with CaPI
You can use the provided compiler wrappers `clang-inst`/`clang-inst++` to build and instrument program.
Before building, set the environment variable `CAPI_FILTER_FILE` to the name of the generated IC file.

Please note that the compiler wrapper will not automatically link any measurement library.
You will need to define the corresponding build flags yourself.

### Instrumentation with Score-P
The generated filter file is compatible with Score-P.
You can therefore use the Score-P instrumenter instead of the CaPI instrumenter, if desired.
To do this, simply build with `scorep-g++` and set `SCOREP_WRAPPER_INSTRUMENTER_FLAGS="--instrument-filter=<filter-file>"`.

### Dynamic Instrumentation with LLVM XRay
CaPI now provides a runtime library compatible with [LLVM XRay](https://llvm.org/docs/XRay.html).
Instead of using a statically instrumented build for each IC, this enables dynamic instrumentation during program initialization.
With XRay, only one build is required and ICs can be changed without recompilation.

You can toggle this feature by setting `ENABLE_XRAY=ON` on.
When building the target application, you will need to use the Clang compiler and pass the flag `-fxray-instrument`.
XRay uses a pre-filtering mechanism to exclude very small functions. If you want to be able to potentially instrument all functions, you need to pass `-fxray-instruction-threshold=1` as well.
You will then need to link the XRay-compatible CaPI runtime library into your executable by adding the following:
`-Wl,--whole-archive <capi_build_dir>/lib/xray/libcapixray_<capi_interface>.a -Wl,--no-whole-archive`.

There are currently three different tool interfaces implemented in the following libraries:
- `libcapixray_gnu.a`: Compatible with `-finstrument-functions`. Calls `__cyg_profile_func_enter` on enter and ``__cyg_profile_func_exit` on exit.
- `libcapixray_scorep.a`: Compatible with the GNU interface of Score-P.
- `libcapixray_talp.a`: Interface for the TALP tool.

The upstream version of LLVM does currently not support XRay instrumentation of shared libraries.
If you need this feature, you can use [this fork of LLVM 13](https://github.com/sebastiankreutzer/llvm-project-xray-dso).  
The feature is enabled by passing the additional flag `-fxray-enable-shared` when building your application.

## Ongoing development
We are currently evaluating a new call-graph analysis method that runs at link-time.
A LLVM linker plugin embeds the call-graph into the binary.
This allows the CaPI runtime to load the call-graph dynamically and to merge call-graphs of shared libraries on the fly.
This work is currently in development and will be made public in the near future.



