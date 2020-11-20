#include "gazosan.h"

namespace gazosan {

// public
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

// public
void create_dir(const std::string& dir) {
  std::string str;
  for (const auto& d : gazosan::split(dir, "/")) {
    if (d == "." || d == "..") {
      continue;
    }

    str += d + "/";
    mkdir(str.c_str(), S_IRWXU);
  }
}

}  // namespace gazosan
