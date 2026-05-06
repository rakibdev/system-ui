module;
#include <spawn.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

export module run;

import std;

import log;

export std::string run(std::string& command) {
  std::array<char, 128> buffer;
  std::string result;
  std::unique_ptr<std::FILE, decltype(&pclose)> pipe(popen(command.c_str(), "r"), pclose);
  if (!pipe) {
    Log::error("popen \"" + command + "\" failed.");
    return "";
  }
  while (std::fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
    result += buffer.data();
  return result;
}

export void runNewProcess(const std::string& command) {
  std::vector<char*> args;
  std::string arg;
  bool inQuotes = false;
  for (std::size_t i = 0; i < command.length(); i++) {
    auto character = command[i];
    if (character == ' ' && !inQuotes) {
      args.emplace_back(strdup(arg.c_str()));
      arg.clear();
    } else if (character == '"' || character == '\'')
      inQuotes = !inQuotes;
    else
      arg += character;
  }
  if (!arg.empty()) args.emplace_back(strdup(arg.c_str()));
  args.emplace_back(nullptr);

  std::signal(SIGCHLD, SIG_IGN);
  pid_t pid;
  int status = posix_spawnp(&pid, args[0], nullptr, nullptr, args.data(), environ);
  if (status != 0) Log::error("posix_spawnp \"" + command + "\" failed.");

  for (char* arg : args) std::free(arg);
}
