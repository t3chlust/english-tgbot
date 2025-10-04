#ifndef UTILS_H
#define UTILS_H
#include <sstream>
#include <string>
#include <vector>
#include <time.h>

inline std::vector<std::string> getTextArguments(std::string text) {
  std::stringstream s(text);
  std::string t;
  std::vector<std::string> result;
  while (getline(s, t, ' ')) {
    result.push_back(t);
  }
  return result;
}

inline std::string getDateTime() {
  time_t mytime = time(NULL);
  struct tm *now = localtime(&mytime);
  char str[20];
  strftime(str, sizeof(str), "%D %T", now);
  return str;
}
#endif
