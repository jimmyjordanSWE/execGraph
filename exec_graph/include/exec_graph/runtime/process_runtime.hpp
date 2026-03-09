#pragma once

#include <functional>
#include <string>
#include <vector>

namespace exec_graph::runtime {

struct ProcessSpec {
    std::vector<std::string> argv;
};

struct ProcessResult {
    int pid;
    int exit_code;
    bool exited;
    bool signaled;
    int signal_number;
    std::string stdout_data;
    std::string stderr_data;
};

struct ProcessEvent {
    std::string name;
    std::string subject;
    int pid;
    int exit_code;
    int signal_number;
    std::string terminal_cause;
    std::string stream_name;
    int byte_count;
    std::string stdout_excerpt;
    std::string stderr_excerpt;
};

std::vector<ProcessSpec> load_workflow(const std::string& workflow_path);
ProcessResult run_process(const ProcessSpec& spec, const std::string& stdin_data);
ProcessResult run_process(const std::string& subject,
                          const ProcessSpec& spec,
                          const std::string& stdin_data,
                          const std::function<void(const ProcessEvent&)>& event_sink);
std::string run_workflow(const std::vector<ProcessSpec>& workflow);
std::string run_workflow(
    const std::vector<ProcessSpec>& workflow,
    const std::function<void(const ProcessEvent&)>& event_sink
);
ProcessEvent build_process_started_event(const std::string& subject, int pid);
ProcessEvent build_process_stream_event(const std::string& subject,
                                        int pid,
                                        const std::string& stream_name,
                                        const std::string& data);
ProcessEvent build_process_event(const std::string& subject, const ProcessResult& result);
std::string format_process_failure(const std::string& subject,
                                   const ProcessSpec& spec,
                                   const ProcessResult& result);
std::string render_process_event_json(const ProcessEvent& event);

}  // namespace exec_graph::runtime
