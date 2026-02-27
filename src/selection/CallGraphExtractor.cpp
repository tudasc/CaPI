//
// Created by sebastian on 27.02.26.
//

#include "capi/selection/CallGraphExtractor.h"

#include "capi/support/CGUtils.h"

#include "llvm/Support/Program.h"

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
  std::filesystem::path tmpFile = std::filesystem::temp_directory_path() / "ldd_output.txt";

  // Redirect stdout of ldd to the temp file
  SmallVector<std::optional<StringRef>, 3> redirects = {std::nullopt, tmpFile.string(), std::nullopt};

  SmallVector<StringRef, 4> args;
  args.push_back(binary.string());

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
    llvm::errs() << "Warning: failed to run ldd on " << binary << "\n";
    return outputLines;
  }

  // Read temp file
  std::ifstream ifs(tmpFile);
  std::string line;
  while (std::getline(ifs, line)) {
    outputLines.push_back(line);
  }

  std::filesystem::remove(tmpFile);
  return outputLines;
}

static void extractAndMergeDependencies(const std::filesystem::path& binary,
                 std::unique_ptr<metacg::Callgraph>& mainCG,
                 std::unordered_set<std::string>& seen) {

  using namespace llvm;

  std::string absPath = std::filesystem::absolute(binary).string();
  if (seen.count(absPath))
    return; // already processed
  seen.insert(absPath);

  // Run ldd and parse dependencies
  auto lines = runLddAndCapture(binary);
  std::regex libRegex(R"(=>\s*(/[^ ]+))"); // captures "/path/to/lib.so"

  for (const auto& line : lines) {
    std::smatch match;
    if (std::regex_search(line, match, libRegex)) {
      std::filesystem::path depPath(match[1].str());

      // Extract callgraph from this binary
      auto cg = extractCallGraph(depPath);
      if (cg) {
        mainCG->merge(*cg, metacg::MergeByName{});
      } else {
        logWarn() << "No metacg section in " << binary << "\n";
      }

      extractAndMergeDependencies(depPath, mainCG, seen);
    }
  }
}

std::unique_ptr<metacg::Callgraph>
extractAndAssembleFullCallGraph(const std::filesystem::path& binary) {

  auto mainCG = extractCallGraph(binary);
  if (!mainCG) {
    logError() << "Failed to extract callgraph from "
           << binary.string() << "\n";
    return nullptr;
  }

  // Keep track of already processed libraries
  std::unordered_set<std::string> seen;

  // Process dependencies recursively
  extractAndMergeDependencies(binary, mainCG, seen);

  return mainCG;
}



}