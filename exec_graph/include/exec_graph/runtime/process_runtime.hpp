#pragma once

#include <string>
#include <vector>

namespace exec_graph::runtime {

struct ProcessSpec {
    std::vector<std::string> argv;
};

struct ProcessResult {
    int exit_code;
    std::string stdout_data;
};

std::vector<ProcessSpec> load_workflow(const std::string& workflow_path);
ProcessResult run_process(const ProcessSpec& spec, const std::string& stdin_data);
std::string run_workflow(const std::vector<ProcessSpec>& workflow);

}  // namespace exec_graph::runtime
