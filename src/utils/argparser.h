#pragma once

#include <algorithm>
#include <sstream>
#include <string>
#include <vector>

struct ArgParser {
  std::vector<std::string> args;

  ArgParser(int argc, char* argv[]) {
    for (int i = 1; i < argc; i++) {
      args.push_back(argv[i]);
    }
  }

  ArgParser(const std::string& input) {
    std::stringstream stream(input);
    std::string arg;
    while (stream >> arg) args.push_back(arg);
  }

  std::string operator[](size_t index) const {
    return index < args.size() ? args[index] : "";
  }

  bool has(std::string_view arg) const {
    return std::find(args.begin(), args.end(), arg) != args.end();
  }

  std::string value(const std::string& flag,
                    const std::string& defaultValue = "") const {
    auto it = std::find(args.begin(), args.end(), flag);
    if (it != args.end() && ++it != args.end()) {
      std::string val = *it;
      if (val.front() == '=') val = val.substr(1);
      return val;
    }
    return defaultValue;
  }

  template <typename Predicate>
  std::string find(Predicate pred) const {
    auto it = std::find_if(args.begin(), args.end(), pred);
    return it != args.end() ? *it : "";
  }
};
