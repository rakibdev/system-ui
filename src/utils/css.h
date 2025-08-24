#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "../config.h"
#include "../style.h"
#include "file.h"

class CssManager {
 private:
  struct CssFile {
    std::string path;
    int priority = 0;
  };

  std::vector<CssFile> cssFiles;
  std::vector<std::unique_ptr<FileWatcher>> watchers;
  std::unique_ptr<Style> globalStyle;

  void rebuild();
  void watchFile(std::string_view filePath);

 public:
  CssManager();
  ~CssManager();

  void add(std::string_view filePath, int priority = 0);
};

extern std::unique_ptr<CssManager> cssManager;
