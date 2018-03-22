#pragma once

#include <string>

namespace roerei {
  class structure_exporter {
    static void exec_dependencies(std::string const& export_path);
    static void exec_features(std::string const& export_path);
  public:
    static void exec(std::string const& export_path);
  };
}