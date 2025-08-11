//
// Created by sebastian on 28.10.24.
//

#include "capi/symbol_retriever/SymbolRetriever.h"

#include <iostream>

namespace {
  struct TestData {
    MappedSymTableMap symTables;
  };
  TestData* globalTestData;
}

void check_symbol(void* addr) {
  auto name = findSymbol((uintptr_t) addr, globalTestData->symTables);
  if (name.empty()) {
    std::cout << "not found: " << std::hex << addr << std::dec << std::endl;
  }  else {
    std::cout << name << std::endl;
  }
}

void load_symbols() {
  globalTestData = new TestData;

  auto execPath = getExecPath();
  auto execFilename = execPath.substr(execPath.find_last_of('/') + 1);

  globalTestData->symTables = loadMappedSymTables(execPath, true);
}


