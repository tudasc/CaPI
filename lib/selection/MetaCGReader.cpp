//
// Created by sebastian on 14.10.21.
//

#include "MetaCGReader.h"

//#include "llvm/Support/raw_ostream.h"


#include <fstream>
#include <iostream>
#include <sstream>
#include <cstdio>
#include <cstdlib>
#include <cctype>

namespace capi {

MetaCGReader::FInfoMap::mapped_type &
MetaCGReader::getOrInsert(const std::string &key) {
  if (functions.find(key) != functions.end()) {
    auto &fi = functions[key];
    return fi;
  } else {
    FunctionInfo fi;
    fi.name = key;
    functions.insert({key, fi});
    auto &rfi = functions[key];
    return rfi;
  }
}

void MetaCGReader::parseDemangledNames() {
  std::system("(llvm-cxxfilt < tmpMangledNames.txt) > tmpDemangledNames.txt");

  std::ifstream tmpMangledNamesFileIn("tmpMangledNames.txt");
  std::ifstream tmpDemangledNamesFileIn("tmpDemangledNames.txt");

  std::string mangledName, demangledNameFull;
  while (std::getline(tmpMangledNamesFileIn, mangledName)) {
     FunctionInfo& functionInfo = functions.at(mangledName);

     std::getline(tmpDemangledNamesFileIn, demangledNameFull);
     
     size_t parameterStartPos = demangledNameFull.find('(');
     std::string demangledNoParameters = demangledNameFull.substr(0, parameterStartPos);
     
     // remove templating from name
     int currentNumBrackets = 0;
     demangledNoParameters.erase(std::remove_if(demangledNoParameters.begin(), demangledNoParameters.end(), [&currentNumBrackets](unsigned char x) { 
       if (x == '<') {
         currentNumBrackets++;
         return true;
       } 
       else if (x == '>') {
         currentNumBrackets--;
         return true;
       } 
       else
         return currentNumBrackets > 0;
     }), demangledNoParameters.end());
     
     // remove return value from name
     size_t returnValueEnd = 0;
     for (size_t i = 1; i < demangledNoParameters.size(); i++) {
       if (std::isspace(demangledNoParameters[i]))
         returnValueEnd = i;
     }

     if (returnValueEnd > 0) 
       demangledNoParameters = demangledNoParameters.substr(returnValueEnd+1);

     functionInfo.demangledName = demangledNoParameters;

     // extract parameters
     std::string parameterString = demangledNameFull.substr(parameterStartPos+1, demangledNameFull.size() - parameterStartPos - 1);
     int bracketDepth = 0;
     size_t currentStartPos = 0;
     for (int i = 0; i < parameterString.size(); i++) {
       if (parameterString[i] == '<' || parameterString[i] == '(') {
         bracketDepth++;
         continue;
       }
       else if (parameterString[i] == '>' || parameterString[i] == ')') {
         bracketDepth--;
         if (bracketDepth < 0 && parameterString.size() > 1) {
           functionInfo.parameters.push_back(parameterString.substr(currentStartPos, i - currentStartPos));
           break;
         }
         continue;
       }

       if (parameterString[i] == ',' && bracketDepth == 0) {
         functionInfo.parameters.push_back(parameterString.substr(currentStartPos, i - currentStartPos));
         currentStartPos = i + 2;
       }
     }
  }

  tmpMangledNamesFileIn.close();
  tmpDemangledNamesFileIn.close();
  std::remove("tmpMangledNames.txt");
  std::remove("tmpDemangledNames.txt");
}

bool MetaCGReader::read() {
  functions.clear();
  json j;
  {
    std::ifstream in(inputFile);
    if (!in.is_open()) {
      std::cerr << "Error: Opening file failed: " << inputFile << "\n";
      return false;
    }
    if (!json::accept(in)) {
      std::cerr << "Error: Invalid JSON file\n";
      return false;
    }
    in.clear();
    in.seekg(0, std::ios::beg);
    in >> j;
  }

//  std::stringstream ss(input);
//  ss >> j;

  // TODO: Ignoring version information etc. for now

  auto jsonCG = j["_CG"];
  if (jsonCG.is_null()) {
    std::cerr << "Error: The call graph entry in the MetaCG file is null.\n";
    return false;
  }

  std::ofstream tmpMangledNamesFileOut("tmpMangledNames.txt");

  for (auto it = jsonCG.begin(); it != jsonCG.end(); ++it) {
    auto &fi = getOrInsert(it.key());

    tmpMangledNamesFileOut << it.key() << "\n";

    auto jCallees = it.value()["callees"];
    if (!jCallees.is_null()) {
      std::for_each(jCallees.begin(), jCallees.end(), [&fi](auto &jCallee) {
        fi.callees.push_back(jCallee.template get<std::string>());
      });
    }

    auto jCallers = it.value()["callers"];
    if (!jCallers.is_null()) {
      std::for_each(jCallers.begin(), jCallers.end(), [&fi](auto &jCaller) {
        fi.callers.push_back(jCaller.template get<std::string>());
      });
    }

    auto jVirtual = it.value()["isVirtual"];
    if (jVirtual.is_boolean()) {
      fi.isVirtual = jVirtual.template get<bool>();
    }

    auto jOverrides = it.value()["overrides"];
    if (!jOverrides.is_null()) {
      std::for_each(jOverrides.begin(), jOverrides.end(), [&fi](auto &jOverrideName) {
        fi.overrides.push_back(jOverrideName.template get<std::string>());
      });
    }

    auto jOverridenBy = it.value()["overridenBy"];
    if (!jOverridenBy.is_null()) {
      std::for_each(jOverridenBy.begin(), jOverridenBy.end(), [&fi](auto &jOverridenby_name) {
        fi.overridenBy.push_back(jOverridenby_name.template get<std::string>());
      });
    }

    auto jMeta = it.value()["meta"];
    fi.metaData = jMeta;
    if (!jMeta.is_null()) {

      auto jPointerCall = jMeta["containsPointerCall"];
      if (jPointerCall.is_boolean()) {
        fi.containsPointerCall = jPointerCall.template get<bool>();
      }
      auto jFileProps = jMeta["fileProperties"];
      if (!jFileProps.is_null()) {
        auto jOrigin = jFileProps["origin"];
        auto jSystemInclude = jFileProps["systemInclude"];
        if (jOrigin.is_string()) {
          fi.fileName = jOrigin.get<std::string>();
        }
        if (jSystemInclude.is_boolean()) {
          fi.definedInSystemInclude = jSystemInclude.get<bool>();
        }
      }
      auto jInlined = jMeta["isInlined"];
      if (jInlined.is_boolean()) {
        fi.isInlined = jInlined.get<bool>();
      }
      auto jVCalls = jMeta["virtualCalls"];
      if (jVCalls.is_null()) {
        // Backwards compatibility: if there is no virtual call data, assume that every call is virtual
        fi.virtualCalls = fi.callees;
      } else {
        std::for_each(jVCalls.begin(), jVCalls.end(), [&fi](auto &jVCallName) {
          fi.virtualCalls.push_back(jVCallName.template get<std::string>());
        });
      }
    }
  }
  tmpMangledNamesFileOut.close();

  parseDemangledNames();
  
  return true;
}
}
