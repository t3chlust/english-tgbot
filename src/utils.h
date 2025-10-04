#ifndef UTILS_H
#define UTILS_H

#include <sstream>
#include <string>
#include <vector>

inline std::vector<std::string> getTextArguments(std::string text) {
  std::stringstream s(text);
  std::string t;
  std::vector<std::string> result;
  while (getline(s, t, ' ')) {
    result.push_back(t);
  }
  return result;
}

#endif //UTILS_H
