// Copyright (c) 2024 Rakib (github.com/rakibdev/system-ui)
// SPDX-License-Identifier: MPL-2.0

#include "build.h"

#include "utils.h"

void patch(std::string& target, const std::string& source,
           std::map<std::string, std::string>& variables, std::string& error) {
  int targetIndex = 0;
  int sourceIndex = 0;
  int targetReplaceStart = -1;
  int sourceVariableStart = -1;
  std::string sourceVariable = "";

  while (targetIndex < target.length()) {
    if (target[targetIndex] == ' ') {
      targetIndex++;
      continue;
    }
    if (source[sourceIndex] == ' ') {
      sourceIndex++;
      continue;
    }
    if (targetReplaceStart != -1) {
      int replaceEnd = -1;
      if (target[targetIndex] == source[sourceIndex])
        replaceEnd = targetIndex - 1;
      else if (targetIndex == (target.length() - 1))
        replaceEnd = targetIndex;
      if (replaceEnd > -1) {
        int length = replaceEnd - (targetReplaceStart - 1);
        target.replace(targetReplaceStart, length, sourceVariable);
        targetIndex = targetReplaceStart + (sourceVariable.length() - 1);
        targetReplaceStart = -1;
      }

      targetIndex++;
    } else if (sourceVariableStart != -1) {
      bool end = source[sourceIndex] == '}';
      if (end) {
        if (variables.find(sourceVariable) == variables.end()) {
          error = "\"" + sourceVariable + "\" is not defined";
          break;
        } else {
          sourceVariable = variables[sourceVariable];
          sourceVariableStart = -1;
          targetReplaceStart = targetIndex;
        }
      } else if (sourceIndex == source.length() - 1) {
        error = "\"}\" template closing missing.";
        break;
      } else
        sourceVariable += source[sourceIndex];

      sourceIndex++;
    } else if (target[targetIndex] == source[sourceIndex]) {
      targetIndex++;
      sourceIndex++;
    } else {
      bool templateExpression =
          source[sourceIndex] == '$' && source[sourceIndex + 1] == '{';
      if (templateExpression) {
        sourceIndex += 2;  // skip {
        sourceVariableStart = sourceIndex;
      } else
        break;
    }
  }
}

void build(const UserConfig::Build& config) {
  std::string targetPath = getAbsolutePath(config.target, CONFIG_DIR);
  std::string sourcePath = getAbsolutePath(config.source, CONFIG_DIR);
  std::ifstream targetFile(targetPath);
  std::ifstream sourceFile(sourcePath);
  if (!targetFile.is_open()) {
    Log::error("Build target file not found: " + targetPath);
    return;
  }
  if (!sourceFile.is_open()) {
    Log::error("Build source file not found: " + sourcePath);
    return;
  }
  if (config.action.empty()) {
    Log::error("Build action missing: " + targetPath);
    return;
  }
  if (config.action == "patch") {
    std::map<std::string, std::string> variables;
    for (const auto& [key, value] : appData.get().theme)
      variables[key] = value.substr(1);

    std::string line;
    std::vector<std::string> source;
    while (std::getline(sourceFile, line)) source.emplace_back(line);

    std::vector<std::string> target;
    while (std::getline(targetFile, line)) {
      for (const auto& sourceLine : source) {
        std::string error;
        patch(line, sourceLine, variables, error);
        if (!error.empty()) Log::error(error + " in " + sourceLine);
      }
      target.emplace_back(line);
    }

    std::ofstream outfile(targetPath, std::ofstream::trunc);
    for (const auto& line : target) outfile << line << "\n";
  }
}

void Build::start() {
  auto& builds = userConfig.get().builds;
  for (const auto& config : builds) build(config);
}