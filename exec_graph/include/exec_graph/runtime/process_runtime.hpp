#pragma once

#include <string>
#include <vector>

namespace exec_graph::runtime {

struct ProcessSpec {
    std::vector<std::string> argv;
};

struct ProcessResult {
    int exit_code;
    bool exited;
    bool signaled;
    int signal_number;
    std::string stdout_data;
    std::string stderr_data;
};

std::vector<ProcessSpec> load_workflow(const std::string& workflow_path);
ProcessResult run_process(const ProcessSpec& spec, const std::string& stdin_data);
std::string run_workflow(const std::vector<ProcessSpec>& workflow);
std::string format_process_failure(const std::string& subject,
                                   const ProcessSpec& spec,
                                   const ProcessResult& result);

}  // namespace exec_graph::runtime
