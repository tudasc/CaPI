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
cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -DDLB_DIR=/path/to/dlb -DSCOREP_PATH=/path/to/scorep .. 
ninja
```
Options
- `ENABLE_TALP=ON/OFF`: Enable/Disable support for TALP interface. Default is `ON`.

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
capi -i <selection_spec> callgraph.ipcg
```

The specification consists of a sequence of selector definitions, which can be named or anonymous.
Each definition takes a list of parameters.
Available parameter types are strings (enclosed in double quotes), booleans (true/false),  integers and floating point numbers.
In addition, most selectors expect another selector definition as input.
These can be either in-place definitions or references to other named selectors, marked with `%`.
The selector `%%` is pre-defined and refers to an instance of the `EverythingSelector`, which selects every function in the CG.

The last definition in the sequence is used as the entry point for the selection pipeline.


Example for MPI call path selection:
```
mpi=onCallPathTo(byName(\"MPI_.*\", %%))
```

Example for MPI call path selection combined with excluding functions from directory "foo", as well as inlined functions:

```
mpi=onCallPathTo(byName(\"MPI_.*\", %%)) exclude=join(byPath(\"foo/.*\", %%), inlineSpecified(%%)) subtract(%mpi, %exclude)
```

List of available selectors:

| Name                    | Parameters   | Selector inputs | Example                             | Explanation                                                     |
|-------------------------|--------------|-----------------|-------------------------------------|-----------------------------------------------------------------|
| byName                  | regex string | 1               | `byName("foo.*", %%)`               | Selects functions with names starting with "foo".               |
| byPath                  | regex string | 1               | `byPath("foo/.*", %%)`              | Selects functions contained in directory "foo".                 |
| inlineSpecified         | -            | 1               | `inlineSpecified(%%)`               | Selects functions marked as `inline`.                           |
| onCallPathTo            | -            | 1               | `onCallPathTo(byName("foo", %%))`   | Selects functions in the call chain to function "foo".          |
| onCallPathFrom          | -            | 1               | `onCallPathFrom(byName("foo", %%))` | Selects functions in the call chain from function "foo".        |
| inSystemHeader          | -            | 1               | `inSystemHeader(%%)`                | Selects functions defined in system headers.                    |
| containsUnresolvedCalls | -            | 1               | `containsUnresolvedCalls(%%)`       | Selects functions containing calls to unknown target functions. |
| join                    | -            | 2               | `join(%A, %B)`                      | Union of the two input sets.                                    |
| intersect               | -            | 2               | `intersect(%A, %B)`                 | Intersection of the two input sets.                             |
| subtract                | -            | 2               | `subtract(%A, %B)`                  | Complement of the two input sets.                               |

### Instrumentation with CaPI
You can use the provided compiler wrappers `clang-inst`/`clang-inst++` to build and instrument program.
Before building, set the environment variable `INST_FILTER_FILE` to the name of the generated IC file.

Please note that the compiler wrapper will not automatically link any measurement library.
You will need to define the corresponding build flags yourself.

### Instrumentation with Score-P
The generated filter file is compatible with Score-P.
You can therefore use the Score-P instrumenter instead of the CaPI instrumenter, if desired.
To do this, simply build with `scorep-g++` and set `SCOREP_WRAPPER_INSTRUMENTER_FLAGS="--instrument-filter=<filter-file>"`.




