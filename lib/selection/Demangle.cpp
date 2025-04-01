//
// Created by sebastian on 01.04.25.
//

#include "Demangle.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <cstdio>
#include <cstdlib>
#include <cctype>

#include "../support/Logging.h"
#include "metadata/CaPIMD.h"
#include "Callgraph.h"

namespace capi {

void demangleNames(metacg::Callgraph &cg) {

  std::ofstream tmpMangledNamesFileOut("tmpMangledNames.txt");
  for (auto& [id, node] : cg.getNodes()) {
    tmpMangledNamesFileOut << node->getFunctionName() << "\n";
  }
  tmpMangledNamesFileOut.close();

  std::system("(llvm-cxxfilt < tmpMangledNames.txt) > tmpDemangledNames.txt");

  std::ifstream tmpMangledNamesFileIn("tmpMangledNames.txt");
  std::ifstream tmpDemangledNamesFileIn("tmpDemangledNames.txt");

  std::string mangledName, demangledNameFull;
  while (std::getline(tmpMangledNamesFileIn, mangledName)) {

    auto node = cg.getNode(mangledName);
    if (!node) {
      logError() << "Node for function " << mangledName << " does not exist!\n";
      continue;
    }
    auto md = node->getOrCreateMD<CaPIMD>();

    auto& info = md->info;

    std::getline(tmpDemangledNamesFileIn, demangledNameFull);

    size_t parameterStartPos = demangledNameFull.find('(');
    std::string demangledNoParameters =
        demangledNameFull.substr(0, parameterStartPos);

    // remove templating from name
    int currentNumBrackets = 0;
    demangledNoParameters.erase(
        std::remove_if(demangledNoParameters.begin(),
                       demangledNoParameters.end(),
                       [&currentNumBrackets](unsigned char x) {
                         if (x == '<') {
                           currentNumBrackets++;
                           return true;
                         } else if (x == '>') {
                           currentNumBrackets--;
                           return true;
                         } else
                           return currentNumBrackets > 0;
                       }),
        demangledNoParameters.end());

    // remove return value from name
    size_t returnValueEnd = 0;
    for (size_t i = 1; i < demangledNoParameters.size(); i++) {
      if (std::isspace(demangledNoParameters[i]))
        returnValueEnd = i;
    }

    if (returnValueEnd > 0)
      demangledNoParameters = demangledNoParameters.substr(returnValueEnd + 1);

    info.demangledName = demangledNoParameters;

    // extract parameters
    std::string parameterString = demangledNameFull.substr(
        parameterStartPos + 1,
        demangledNameFull.size() - parameterStartPos - 1);
    int bracketDepth = 0;
    size_t currentStartPos = 0;
    for (int i = 0; i < parameterString.size(); i++) {
      if (parameterString[i] == '<' || parameterString[i] == '(') {
        bracketDepth++;
        continue;
      } else if (parameterString[i] == '>' || parameterString[i] == ')') {
        bracketDepth--;
        if (bracketDepth < 0 && parameterString.size() > 1) {
          info.parameters.push_back(
              parameterString.substr(currentStartPos, i - currentStartPos));
          break;
        }
        continue;
      }

      if (parameterString[i] == ',' && bracketDepth == 0) {
        info.parameters.push_back(
            parameterString.substr(currentStartPos, i - currentStartPos));
        currentStartPos = i + 2;
      }
    }
  }

  tmpMangledNamesFileIn.close();
  tmpDemangledNamesFileIn.close();
  std::remove("tmpMangledNames.txt");
  std::remove("tmpDemangledNames.txt");
}

}