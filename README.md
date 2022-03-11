# CaPI: Compiler-assisted Performance Instrumentation

Selective instrumentation tool based on InstRO.

This project is currently in a pre-release state, frequent changes to the code and build config can be expected.

## Requirements

- LLVM 10
- ScoreP 7
- CMake >=3.17

If you use Score-P for profiling and want to measure shared libraries, we suggest using the [Score-P Symbol Injector](https://github.com/sebastiankreutzer/scorep-symbol-injector) library.

## Build
You can build CaPI as follows ([Ninja](https://github.com/ninja-build/ninja) is not required, you can use make instead).
```
mkdir build && cd build
cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release 
ninja
```

## Usage

CaPI relies on [MetaCG](https://github.com/tudasc/MetaCG) for its whole-program call graph analysis.

In order to apply CaPI, you first need to install MetaCG and run it on your target application.
See the MetaCG README for instructions.

You can then run CaPI to generate the instrumentation configuration (IC).
Currently, only pre-defined selection pipelines can be used (a parser for user-defined pipelines is WIP).

The `MPI` preset can be used to track all functions that are on a call-path to MPI.
It can be applied as follows:
```
capi -p MPI callgraph.ipcg
```
This will create the IC in `callgraph.filt`.
You can use the provided compiler wrappers `clang-inst`/`clang-inst++` to build and instrument program.
Before building, set the environment variable `INST_FILTER_FILE` to the name of the generated IC file.

Please note that the compiler wrapper will not automatically link any measurement library.
You will need to define the corresponding build flags yourself.

### Instrumentation with Score-P
The generated filter file is compatible with Score-P.
You can therefore use the Score-P instrumenter instead of the CaPI instrumenter, if desired.
To do this, simply build with `scorep-g++` and set `SCOREP_WRAPPER_INSTRUMENTER_FLAGS="--instrument-filter=<filter-file>"`.




