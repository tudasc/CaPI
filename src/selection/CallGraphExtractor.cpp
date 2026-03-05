//
// Created by sebastian on 27.02.26.
//

#include "capi/selection/CallGraphExtractor.h"

#include "capi/support/CGUtils.h"

#include "cage/generator/DynamicLinkagePolicy.h"

#include "llvm/Support/Program.h"
#include "llvm/Object/ObjectFile.h"
#include "llvm/Object/ELFObjectFile.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/WithColor.h"

#include <filesystem>
#include <memory>
#include <string>
#include <vector>
#include <sstream>
#include <regex>
#include <unordered_set>


namespace capi {

std::unique_ptr<metacg::Callgraph> extractCallGraph(const std::filesystem::path& binary) {

  using namespace llvm;
  using namespace llvm::object;

  // Load file into memory
  auto bufferOrErr = MemoryBuffer::getFile(binary.string());
  if (!bufferOrErr) {
    logError() << "extract: failed to open file '"
           << binary.string() << "'\n";
    return nullptr;
  }

  std::unique_ptr<MemoryBuffer> buffer = std::move(*bufferOrErr);

  // Create object file
  auto objOrErr =
      ObjectFile::createObjectFile(buffer->getMemBufferRef());

  if (!objOrErr) {
    logError() << "extract: not a valid object file: "
           << binary.string() << "\n";
    logAllUnhandledErrors(objOrErr.takeError(), errs());
    return nullptr;
  }

  std::unique_ptr<ObjectFile> obj = std::move(*objOrErr);

  // Ensure this is ELF
  if (!isa<ELFObjectFileBase>(obj.get())) {
    logError() << "extract: file is not an ELF binary: "
           << binary.string() << "\n";
    return nullptr;
  }

  // Search for section "metacg"
  for (const SectionRef& section : obj->sections()) {

    auto nameOrErr = section.getName();
    if (!nameOrErr) {
      logAllUnhandledErrors(nameOrErr.takeError(), errs());
      return nullptr;
    }

    if (*nameOrErr == "metacg") {

      auto contentsOrErr = section.getContents();
      if (!contentsOrErr) {
        logAllUnhandledErrors(contentsOrErr.takeError(), errs());
        return nullptr;
      }

      std::string metacgContent = contentsOrErr->str();

      auto graph = loadGraphFromStr(metacgContent.c_str());
      return graph;
    }
  }

  // Section not found -> no call graph embedded

  return nullptr;

}

static std::vector<std::string>
runLddAndCapture(const std::filesystem::path &binary) {
  using namespace llvm;

  std::vector<std::string> outputLines;

  // Create a temporary file to capture stdout
    SmallString<128> tmpPath;
    int tmpFD;
    if (sys::fs::createTemporaryFile("ldd-output", "txt", tmpFD, tmpPath)) {
        errs() << "Failed to create temporary file for ldd\n";
        return outputLines;
    }

    // Close FD — ExecuteAndWait will reopen via path
    sys::fs::closeFile(tmpFD);


    // Redirect stdout of ldd to the temp file
  SmallVector<std::optional<StringRef>, 3> redirects = {std::nullopt, StringRef(tmpPath), std::nullopt};

  auto binaryStr = binary.string();
  SmallVector<StringRef, 4> args;
  args.push_back("ldd");
  args.push_back(binaryStr);

  bool execFailed = false;
  int rc = sys::ExecuteAndWait(
      "/usr/bin/ldd",
      args,
      /*Env=*/std::nullopt,
      /*Redirects=*/redirects,
      /*SecondsToWait=*/0,
      /*MemoryLimit=*/0,
      /*ErrMsg=*/nullptr,
      &execFailed
  );

  if (rc != 0 || execFailed) {
    logWarn() << "Warning: failed to run ldd on " << binary << "\n";
    return outputLines;
  }

  // Read temp file
  std::ifstream ifs(tmpPath.c_str());
  std::string line;
  while (std::getline(ifs, line)) {
      outputLines.push_back(line);
  }

  std::filesystem::remove(tmpPath.c_str());
  return outputLines;
}

static void extractAndMergeDependencies(const std::filesystem::path& executable,
                 std::unique_ptr<metacg::Callgraph>& mainCG) {

  using namespace llvm;

  std::vector<std::filesystem::path> workList;
  std::unordered_set<std::filesystem::path> seen;
  workList.push_back(executable);
  seen.insert(executable);

  while (!workList.empty()) {
      auto binary = workList.back();
      workList.pop_back();

      // Run ldd and parse dependencies
      auto lines = runLddAndCapture(binary);
      std::regex libRegex(R"(=>\s*(/[^ ]+))"); // captures "/path/to/lib.so"

      for (const auto& line : lines) {
          std::smatch match;
          if (std::regex_search(line, match, libRegex)) {
              std::filesystem::path depPath(match[1].str());

              if (seen.contains(depPath))
                  continue;


              // Extract callgraph from this binary
              auto cg = extractCallGraph(depPath);
              if (cg) {
                  logInfo() << "Merging with call graph from dependency " << depPath << "\n";
                  mainCG->merge(*cg, cage::DynamicLinkagePolicy{});
              } else {
//                  logWarn() << "Could not extract call graph from " << depPath << "\n";
              }

              seen.insert(depPath);
              workList.push_back(depPath);
          }
      }

  }
}

std::unique_ptr<metacg::Callgraph>
extractAndAssembleFullCallGraph(const std::filesystem::path& binary) {

  auto mainCG = extractCallGraph(binary);
  if (!mainCG) {
      logError() << "Failed to extract callgraph from "
                 << binary << "\n";
      return nullptr;
  }

  // Process dependencies recursively
  extractAndMergeDependencies(binary, mainCG);

  return mainCG;
}



}