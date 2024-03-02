// Copyright (c) 2024 Rakib (github.com/rakibdev/system-ui)
// SPDX-License-Identifier: MPL-2.0

#include "patch.h"

#include "../utils.h"

void patchLine(std::string& target, const std::string& source,
               const std::map<std::string, std::string>& variables,
               std::string& error) {
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
          error = "\"" + sourceVariable + "\" value is not defined.";
          break;
        } else {
          sourceVariable = variables.at(sourceVariable);
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

void patch(const std::string& source, const std::string& target,
           std::string& error) {
  std::ifstream sourceFile(source);
  std::ifstream targetFile(target);
  if (!sourceFile.is_open()) return Log::error("File not found: " + source);
  if (!targetFile.is_open()) return Log::error("File not found: " + target);

  std::map<std::string, std::string> variables;
  for (const auto& [key, value] : appData.get().theme)
    variables[key] = value.substr(1);

  std::string line;

  std::vector<std::string> sourceContent;
  while (std::getline(sourceFile, line)) sourceContent.emplace_back(line);

  std::vector<std::string> result;
  while (std::getline(targetFile, line)) {
    for (const auto& sourceLine : sourceContent) {
      patchLine(line, sourceLine, variables, error);
      if (!error.empty()) return;
    }
    result.emplace_back(line);
  }

  std::ofstream output(target, std::ofstream::trunc);
  for (const auto& line : result) output << line << "\n";
}
