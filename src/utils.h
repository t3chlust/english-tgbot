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

struct Word {
  int64_t chat_id;
  std::string word;
  std::string translation;
  short to_delete;
};
#endif // UTILS_H
