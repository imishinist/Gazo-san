#include "gazosan.h"

namespace gazosan {

std::vector<std::string> split(const std::string& str,
                               const std::string& delim) {
  int first = 0;
  int last = str.find_first_of(delim);

  std::vector<std::string> result;

  while (first < str.size()) {
    std::string subStr(str, first, last - first);

    result.push_back(subStr);

    first = last + 1;
    last = str.find_first_of(delim, first);

    if (last == std::string::npos) {
      last = str.size();
    }
  }

  return result;
}

}  // namespace gazosan
