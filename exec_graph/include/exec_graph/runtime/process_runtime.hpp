#pragma once

#include <functional>
#include <string>
#include <vector>

namespace exec_graph::runtime {

struct ProcessSpec {
    std::vector<std::string> argv;
    std::string working_directory;
    int timeout_ms;
    int graceful_shutdown_ms;
};

struct ProcessResult {
    int pid;
    int exit_code;
    bool exited;
    bool signaled;
    int signal_number;
    std::string terminal_cause;
    std::string stdout_data;
    std::string stderr_data;
};

struct ProcessEvent {
    std::string name;
    std::string subject;
    std::string related_subject;
    int pid;
    int exit_code;
    int signal_number;
    std::string terminal_cause;
    std::string stream_name;
    int byte_count;
    int step_count;
    int completed_step_count;
    int node_count;
    int sink_count;
    int completed_node_count;
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
ProcessEvent build_workflow_started_event(int step_count);
ProcessEvent build_workflow_completed_event(int step_count);
ProcessEvent build_workflow_failed_event(int step_count,
                                         int completed_step_count,
                                         const std::string& failed_step,
                                         const ProcessResult& result);
ProcessEvent build_workflow_step_started_event(const std::string& step_name,
                                               int step_count,
                                               int completed_step_count);
ProcessEvent build_workflow_step_completed_event(const std::string& step_name,
                                                 int step_count,
                                                 int completed_step_count,
                                                 const ProcessResult& result);
ProcessEvent build_workflow_step_failed_event(const std::string& step_name,
                                              int step_count,
                                              int completed_step_count,
                                              const ProcessResult& result);
ProcessEvent build_process_started_event(const std::string& subject, int pid);
ProcessEvent build_process_stop_requested_event(const std::string& subject, int pid);
ProcessEvent build_process_kill_sent_event(const std::string& subject, int pid);
ProcessEvent build_process_stream_event(const std::string& subject,
                                        int pid,
                                        const std::string& stream_name,
                                        const std::string& data);
ProcessEvent build_process_event(const std::string& subject, const ProcessResult& result);
ProcessEvent build_graph_started_event(int node_count, int sink_count);
ProcessEvent build_graph_completed_event(int node_count, int sink_count);
ProcessEvent build_graph_failed_event(int node_count,
                                      int sink_count,
                                      int completed_node_count,
                                      const std::string& failed_node,
                                      const ProcessResult& result);
ProcessEvent build_graph_node_started_event(const std::string& node_name,
                                            int node_count,
                                            int sink_count,
                                            int completed_node_count);
ProcessEvent build_graph_node_completed_event(const std::string& node_name,
                                              int node_count,
                                              int sink_count,
                                              int completed_node_count,
                                              const ProcessResult& result);
ProcessEvent build_graph_node_failed_event(const std::string& node_name,
                                           int node_count,
                                           int sink_count,
                                           int completed_node_count,
                                           const ProcessResult& result);
std::string format_process_failure(const std::string& subject,
                                   const ProcessSpec& spec,
                                   const ProcessResult& result);
std::string render_process_event_json(const ProcessEvent& event);

}  // namespace exec_graph::runtime
