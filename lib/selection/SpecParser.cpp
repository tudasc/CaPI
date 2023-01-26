//
// Created by sebastian on 11.03.22.
//

#include "SpecParser.h"


namespace capi {

std::string stripComments(const std::string& input) {
  std::string out;

  std::stringstream in(input);

  std::string line;
  while (std::getline(in, line)) {
    auto commentStart = line.find_first_of('#');
    if (commentStart > 0) {
      out += line.substr(0, commentStart);
    }
  }
  return out;
}

}