#include "exec_graph/runtime/process_runtime.hpp"

#include <cerrno>
#include <array>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <poll.h>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

namespace exec_graph::runtime {
namespace {

std::vector<std::string> split_whitespace(const std::string& line) {
    std::istringstream stream(line);
    std::vector<std::string> parts;
    for (std::string part; stream >> part;) {
        parts.push_back(part);
    }
    return parts;
}

void close_fd(int fd) {
    if (fd >= 0) {
        close(fd);
    }
}

void write_all(int fd, std::string_view data) {
    const char* cursor = data.data();
    std::size_t remaining = data.size();
    while (remaining > 0) {
        const ssize_t written = write(fd, cursor, remaining);
        if (written < 0) {
            throw std::runtime_error("failed to write process stdin");
        }
        remaining -= static_cast<std::size_t>(written);
        cursor += written;
    }
}

bool read_into_buffer(int fd, std::string& output, const char* stream_name) {
    char buffer[4096];
    const ssize_t count = read(fd, buffer, sizeof(buffer));
    if (count == 0) {
        return true;
    }
    if (count < 0) {
        throw std::runtime_error(std::string("failed to read process ") + stream_name);
    }
    output.append(buffer, static_cast<std::size_t>(count));
    return false;
}

std::pair<std::string, std::string> read_stdout_and_stderr(int stdout_fd, int stderr_fd) {
    std::string stdout_data;
    std::string stderr_data;

    while (stdout_fd >= 0 || stderr_fd >= 0) {
        const short stdout_events = stdout_fd >= 0 ? static_cast<short>(POLLIN | POLLHUP) : static_cast<short>(0);
        const short stderr_events = stderr_fd >= 0 ? static_cast<short>(POLLIN | POLLHUP) : static_cast<short>(0);
        std::array<pollfd, 2> poll_fds{{
            pollfd{stdout_fd, stdout_events, 0},
            pollfd{stderr_fd, stderr_events, 0},
        }};

        const int ready = poll(poll_fds.data(), poll_fds.size(), -1);
        if (ready < 0) {
            throw std::runtime_error("failed to poll process output");
        }

        if (stdout_fd >= 0) {
            bool stdout_eof = false;
            if ((poll_fds[0].revents & POLLIN) != 0) {
                stdout_eof = read_into_buffer(stdout_fd, stdout_data, "stdout");
            }
            if (stdout_eof || ((poll_fds[0].revents & (POLLHUP | POLLERR | POLLNVAL)) != 0 &&
                               (poll_fds[0].revents & POLLIN) == 0)) {
                close_fd(stdout_fd);
                stdout_fd = -1;
            }
        }

        if (stderr_fd >= 0) {
            bool stderr_eof = false;
            if ((poll_fds[1].revents & POLLIN) != 0) {
                stderr_eof = read_into_buffer(stderr_fd, stderr_data, "stderr");
            }
            if (stderr_eof || ((poll_fds[1].revents & (POLLHUP | POLLERR | POLLNVAL)) != 0 &&
                               (poll_fds[1].revents & POLLIN) == 0)) {
                close_fd(stderr_fd);
                stderr_fd = -1;
            }
        }
    }

    return {stdout_data, stderr_data};
}

std::string render_command(const ProcessSpec& spec) {
    std::ostringstream message;
    for (const auto& arg : spec.argv) {
        message << ' ' << arg;
    }
    return message.str();
}

std::string trim_trailing_newlines(std::string text) {
    while (!text.empty() && (text.back() == '\n' || text.back() == '\r')) {
        text.pop_back();
    }
    return text;
}

std::string truncate_excerpt(const std::string& text, const std::size_t limit = 160) {
    auto trimmed = trim_trailing_newlines(text);
    if (trimmed.size() <= limit) {
        return trimmed;
    }
    return trimmed.substr(0, limit) + "...";
}

std::string json_escape(const std::string& value) {
    std::string escaped;
    escaped.reserve(value.size());
    for (const char ch : value) {
        switch (ch) {
            case '\\':
                escaped += "\\\\";
                break;
            case '"':
                escaped += "\\\"";
                break;
            case '\n':
                escaped += "\\n";
                break;
            case '\r':
                escaped += "\\r";
                break;
            case '\t':
                escaped += "\\t";
                break;
            default:
                escaped += ch;
                break;
        }
    }
    return escaped;
}

std::string workflow_step_subject(const std::size_t step_index) {
    return "workflow.step." + std::to_string(step_index + 1);
}

}  // namespace

std::vector<ProcessSpec> load_workflow(const std::string& workflow_path) {
    std::ifstream input(workflow_path);
    if (!input) {
        throw std::runtime_error("failed to open workflow file: " + workflow_path);
    }

    std::vector<ProcessSpec> workflow;
    for (std::string line; std::getline(input, line);) {
        if (line.empty() || line[0] == '#') {
            continue;
        }
        auto tokens = split_whitespace(line);
        if (!tokens.empty()) {
            workflow.push_back(ProcessSpec{std::move(tokens), std::filesystem::absolute(std::filesystem::path(workflow_path)).parent_path().string()});
        }
    }

    if (workflow.empty()) {
        throw std::runtime_error("workflow file contained no runnable commands");
    }

    return workflow;
}

ProcessResult run_process(const ProcessSpec& spec, const std::string& stdin_data) {
    return run_process("", spec, stdin_data, {});
}

ProcessResult run_process(const std::string& subject,
                          const ProcessSpec& spec,
                          const std::string& stdin_data,
                          const std::function<void(const ProcessEvent&)>& event_sink) {
    if (spec.argv.empty()) {
        throw std::runtime_error("attempted to run an empty command");
    }

    int stdin_pipe[2];
    int stdout_pipe[2];
    int stderr_pipe[2];
    if (pipe(stdin_pipe) != 0 || pipe(stdout_pipe) != 0 || pipe(stderr_pipe) != 0) {
        throw std::runtime_error("failed to create pipes");
    }

    const pid_t pid = fork();
    if (pid < 0) {
        throw std::runtime_error("fork failed");
    }

    if (pid == 0) {
        if (!spec.working_directory.empty() && chdir(spec.working_directory.c_str()) != 0) {
            const std::string message = "chdir failed for " + spec.working_directory + ": " + std::strerror(errno) + "\n";
            (void)!write(STDERR_FILENO, message.c_str(), message.size());
            _exit(127);
        }
        dup2(stdin_pipe[0], STDIN_FILENO);
        dup2(stdout_pipe[1], STDOUT_FILENO);
        dup2(stderr_pipe[1], STDERR_FILENO);
        close(stdin_pipe[0]);
        close(stdin_pipe[1]);
        close(stdout_pipe[0]);
        close(stdout_pipe[1]);
        close(stderr_pipe[0]);
        close(stderr_pipe[1]);

        std::vector<char*> argv;
        argv.reserve(spec.argv.size() + 1);
        for (const auto& arg : spec.argv) {
            argv.push_back(const_cast<char*>(arg.c_str()));
        }
        argv.push_back(nullptr);

        execvp(argv[0], argv.data());
        const std::string message = "execvp failed for" + render_command(spec) + ": " + std::strerror(errno) + "\n";
        (void)!write(STDERR_FILENO, message.c_str(), message.size());
        _exit(127);
    }

    if (event_sink) {
        event_sink(build_process_started_event(subject, static_cast<int>(pid)));
    }

    close(stdin_pipe[0]);
    close(stdout_pipe[1]);
    close(stderr_pipe[1]);

    if (!stdin_data.empty()) {
        write_all(stdin_pipe[1], stdin_data);
    }
    close(stdin_pipe[1]);

    const auto [stdout_data, stderr_data] = read_stdout_and_stderr(stdout_pipe[0], stderr_pipe[0]);

    if (event_sink && !stdout_data.empty()) {
        event_sink(build_process_stream_event(subject, static_cast<int>(pid), "stdout", stdout_data));
    }
    if (event_sink && !stderr_data.empty()) {
        event_sink(build_process_stream_event(subject, static_cast<int>(pid), "stderr", stderr_data));
    }

    int status = 0;
    if (waitpid(pid, &status, 0) < 0) {
        throw std::runtime_error("waitpid failed");
    }

    if (WIFEXITED(status)) {
        return ProcessResult{static_cast<int>(pid), WEXITSTATUS(status), true, false, 0, stdout_data, stderr_data};
    }
    if (WIFSIGNALED(status)) {
        return ProcessResult{static_cast<int>(pid), 128 + WTERMSIG(status), false, true, WTERMSIG(status), stdout_data, stderr_data};
    }
    return ProcessResult{static_cast<int>(pid), 1, false, false, 0, stdout_data, stderr_data};
}

std::string run_workflow(const std::vector<ProcessSpec>& workflow) {
    return run_workflow(workflow, {});
}

std::string run_workflow(const std::vector<ProcessSpec>& workflow,
                         const std::function<void(const ProcessEvent&)>& event_sink) {
    std::string current_output;
    const auto step_count = static_cast<int>(workflow.size());
    int completed_step_count = 0;

    if (event_sink) {
        event_sink(build_workflow_started_event(step_count));
    }

    for (std::size_t step_index = 0; step_index < workflow.size(); ++step_index) {
        const auto& process = workflow[step_index];
        const auto subject = workflow_step_subject(step_index);

        if (event_sink) {
            event_sink(build_workflow_step_started_event(subject, step_count, completed_step_count));
        }

        const auto result = run_process(subject, process, current_output, event_sink);
        if (event_sink) {
            event_sink(build_process_event(subject, result));
        }
        if (result.exit_code != 0) {
            if (event_sink) {
                event_sink(build_workflow_step_failed_event(subject, step_count, completed_step_count, result));
                event_sink(build_workflow_failed_event(step_count, completed_step_count, subject, result));
            }
            throw std::runtime_error(format_process_failure(subject, process, result));
        }
        current_output = result.stdout_data;
        ++completed_step_count;

        if (event_sink) {
            event_sink(build_workflow_step_completed_event(subject, step_count, completed_step_count, result));
        }
    }

    if (event_sink) {
        event_sink(build_workflow_completed_event(step_count));
    }
    return current_output;
}

ProcessEvent build_workflow_started_event(const int step_count) {
    ProcessEvent event;
    event.name = "workflow.started";
    event.subject = "workflow";
    event.related_subject = "";
    event.pid = 0;
    event.exit_code = 0;
    event.signal_number = 0;
    event.terminal_cause = "";
    event.stream_name = "";
    event.byte_count = 0;
    event.step_count = step_count;
    event.completed_step_count = 0;
    event.node_count = 0;
    event.sink_count = 0;
    event.completed_node_count = 0;
    event.stdout_excerpt = "";
    event.stderr_excerpt = "";
    return event;
}

ProcessEvent build_workflow_completed_event(const int step_count) {
    ProcessEvent event;
    event.name = "workflow.completed";
    event.subject = "workflow";
    event.related_subject = "";
    event.pid = 0;
    event.exit_code = 0;
    event.signal_number = 0;
    event.terminal_cause = "exit_zero";
    event.stream_name = "";
    event.byte_count = 0;
    event.step_count = step_count;
    event.completed_step_count = step_count;
    event.node_count = 0;
    event.sink_count = 0;
    event.completed_node_count = 0;
    event.stdout_excerpt = "";
    event.stderr_excerpt = "";
    return event;
}

ProcessEvent build_workflow_failed_event(const int step_count,
                                         const int completed_step_count,
                                         const std::string& failed_step,
                                         const ProcessResult& result) {
    ProcessEvent event;
    event.name = "workflow.failed";
    event.subject = "workflow";
    event.related_subject = failed_step;
    event.pid = result.pid;
    event.exit_code = result.exit_code;
    event.signal_number = result.signal_number;
    event.terminal_cause = result.signaled ? "signal" : "exit_non_zero";
    event.stream_name = "";
    event.byte_count = 0;
    event.step_count = step_count;
    event.completed_step_count = completed_step_count;
    event.node_count = 0;
    event.sink_count = 0;
    event.completed_node_count = 0;
    event.stdout_excerpt = truncate_excerpt(result.stdout_data);
    event.stderr_excerpt = trim_trailing_newlines(result.stderr_data);
    return event;
}

ProcessEvent build_workflow_step_started_event(const std::string& step_name,
                                               const int step_count,
                                               const int completed_step_count) {
    ProcessEvent event;
    event.name = "workflow.step.started";
    event.subject = step_name;
    event.related_subject = "workflow";
    event.pid = 0;
    event.exit_code = 0;
    event.signal_number = 0;
    event.terminal_cause = "";
    event.stream_name = "";
    event.byte_count = 0;
    event.step_count = step_count;
    event.completed_step_count = completed_step_count;
    event.node_count = 0;
    event.sink_count = 0;
    event.completed_node_count = 0;
    event.stdout_excerpt = "";
    event.stderr_excerpt = "";
    return event;
}

ProcessEvent build_workflow_step_completed_event(const std::string& step_name,
                                                 const int step_count,
                                                 const int completed_step_count,
                                                 const ProcessResult& result) {
    ProcessEvent event;
    event.name = "workflow.step.completed";
    event.subject = step_name;
    event.related_subject = "workflow";
    event.pid = result.pid;
    event.exit_code = result.exit_code;
    event.signal_number = result.signal_number;
    event.terminal_cause = "exit_zero";
    event.stream_name = "stdout";
    event.byte_count = static_cast<int>(result.stdout_data.size());
    event.step_count = step_count;
    event.completed_step_count = completed_step_count;
    event.node_count = 0;
    event.sink_count = 0;
    event.completed_node_count = 0;
    event.stdout_excerpt = truncate_excerpt(result.stdout_data);
    event.stderr_excerpt = trim_trailing_newlines(result.stderr_data);
    return event;
}

ProcessEvent build_workflow_step_failed_event(const std::string& step_name,
                                              const int step_count,
                                              const int completed_step_count,
                                              const ProcessResult& result) {
    ProcessEvent event;
    event.name = "workflow.step.failed";
    event.subject = step_name;
    event.related_subject = "workflow";
    event.pid = result.pid;
    event.exit_code = result.exit_code;
    event.signal_number = result.signal_number;
    event.terminal_cause = result.signaled ? "signal" : "exit_non_zero";
    event.stream_name = result.stderr_data.empty() ? "stdout" : "stderr";
    event.byte_count = static_cast<int>(result.stderr_data.empty() ? result.stdout_data.size() : result.stderr_data.size());
    event.step_count = step_count;
    event.completed_step_count = completed_step_count;
    event.node_count = 0;
    event.sink_count = 0;
    event.completed_node_count = 0;
    event.stdout_excerpt = truncate_excerpt(result.stdout_data);
    event.stderr_excerpt = trim_trailing_newlines(result.stderr_data);
    return event;
}

ProcessEvent build_process_started_event(const std::string& subject, const int pid) {
    ProcessEvent event;
    event.name = "process.started";
    event.subject = subject;
    event.related_subject = "";
    event.pid = pid;
    event.exit_code = 0;
    event.signal_number = 0;
    event.terminal_cause = "";
    event.stream_name = "";
    event.byte_count = 0;
    event.step_count = 0;
    event.completed_step_count = 0;
    event.node_count = 0;
    event.sink_count = 0;
    event.completed_node_count = 0;
    event.stdout_excerpt = "";
    event.stderr_excerpt = "";
    return event;
}

ProcessEvent build_process_stream_event(const std::string& subject,
                                        const int pid,
                                        const std::string& stream_name,
                                        const std::string& data) {
    ProcessEvent event;
    event.name = "process.output";
    event.subject = subject;
    event.related_subject = "";
    event.pid = pid;
    event.exit_code = 0;
    event.signal_number = 0;
    event.terminal_cause = "";
    event.stream_name = stream_name;
    event.byte_count = static_cast<int>(data.size());
    event.step_count = 0;
    event.completed_step_count = 0;
    event.node_count = 0;
    event.sink_count = 0;
    event.completed_node_count = 0;
    event.stdout_excerpt = stream_name == "stdout" ? truncate_excerpt(data) : "";
    event.stderr_excerpt = stream_name == "stderr" ? truncate_excerpt(data) : "";
    return event;
}

ProcessEvent build_process_event(const std::string& subject, const ProcessResult& result) {
    ProcessEvent event;
    event.subject = subject;
    event.related_subject = "";
    event.pid = result.pid;
    event.exit_code = result.exit_code;
    event.signal_number = result.signal_number;
    event.stream_name = "";
    event.byte_count = 0;
    event.step_count = 0;
    event.completed_step_count = 0;
    event.node_count = 0;
    event.sink_count = 0;
    event.completed_node_count = 0;
    event.stdout_excerpt = truncate_excerpt(result.stdout_data);
    event.stderr_excerpt = trim_trailing_newlines(result.stderr_data);

    if (result.exited && result.exit_code == 0) {
        event.name = "process.completed";
        event.terminal_cause = "exit_zero";
    } else if (result.exited) {
        event.name = "process.failed";
        event.terminal_cause = "exit_non_zero";
    } else if (result.signaled) {
        event.name = "process.killed";
        event.terminal_cause = "signal";
    } else {
        event.name = "process.terminated";
        event.terminal_cause = "unknown";
    }

    return event;
}

ProcessEvent build_graph_started_event(const int node_count, const int sink_count) {
    ProcessEvent event;
    event.name = "graph.started";
    event.subject = "graph";
    event.related_subject = "";
    event.pid = 0;
    event.exit_code = 0;
    event.signal_number = 0;
    event.terminal_cause = "";
    event.stream_name = "";
    event.byte_count = 0;
    event.step_count = 0;
    event.completed_step_count = 0;
    event.node_count = node_count;
    event.sink_count = sink_count;
    event.completed_node_count = 0;
    event.stdout_excerpt = "";
    event.stderr_excerpt = "";
    return event;
}

ProcessEvent build_graph_completed_event(const int node_count, const int sink_count) {
    ProcessEvent event;
    event.name = "graph.completed";
    event.subject = "graph";
    event.related_subject = "";
    event.pid = 0;
    event.exit_code = 0;
    event.signal_number = 0;
    event.terminal_cause = "exit_zero";
    event.stream_name = "";
    event.byte_count = 0;
    event.step_count = 0;
    event.completed_step_count = 0;
    event.node_count = node_count;
    event.sink_count = sink_count;
    event.completed_node_count = node_count;
    event.stdout_excerpt = "";
    event.stderr_excerpt = "";
    return event;
}

ProcessEvent build_graph_failed_event(const int node_count,
                                      const int sink_count,
                                      const int completed_node_count,
                                      const std::string& failed_node,
                                      const ProcessResult& result) {
    ProcessEvent event;
    event.name = "graph.failed";
    event.subject = "graph";
    event.related_subject = failed_node;
    event.pid = result.pid;
    event.exit_code = result.exit_code;
    event.signal_number = result.signal_number;
    event.terminal_cause = result.signaled ? "signal" : "exit_non_zero";
    event.stream_name = "";
    event.byte_count = 0;
    event.step_count = 0;
    event.completed_step_count = 0;
    event.node_count = node_count;
    event.sink_count = sink_count;
    event.completed_node_count = completed_node_count;
    event.stdout_excerpt = truncate_excerpt(result.stdout_data);
    event.stderr_excerpt = trim_trailing_newlines(result.stderr_data);
    return event;
}

ProcessEvent build_graph_node_started_event(const std::string& node_name,
                                            const int node_count,
                                            const int sink_count,
                                            const int completed_node_count) {
    ProcessEvent event;
    event.name = "graph.node.started";
    event.subject = node_name;
    event.related_subject = "graph";
    event.pid = 0;
    event.exit_code = 0;
    event.signal_number = 0;
    event.terminal_cause = "";
    event.stream_name = "";
    event.byte_count = 0;
    event.step_count = 0;
    event.completed_step_count = 0;
    event.node_count = node_count;
    event.sink_count = sink_count;
    event.completed_node_count = completed_node_count;
    event.stdout_excerpt = "";
    event.stderr_excerpt = "";
    return event;
}

ProcessEvent build_graph_node_completed_event(const std::string& node_name,
                                              const int node_count,
                                              const int sink_count,
                                              const int completed_node_count,
                                              const ProcessResult& result) {
    ProcessEvent event;
    event.name = "graph.node.completed";
    event.subject = node_name;
    event.related_subject = "graph";
    event.pid = result.pid;
    event.exit_code = result.exit_code;
    event.signal_number = result.signal_number;
    event.terminal_cause = "exit_zero";
    event.stream_name = "stdout";
    event.byte_count = static_cast<int>(result.stdout_data.size());
    event.step_count = 0;
    event.completed_step_count = 0;
    event.node_count = node_count;
    event.sink_count = sink_count;
    event.completed_node_count = completed_node_count;
    event.stdout_excerpt = truncate_excerpt(result.stdout_data);
    event.stderr_excerpt = trim_trailing_newlines(result.stderr_data);
    return event;
}

ProcessEvent build_graph_node_failed_event(const std::string& node_name,
                                           const int node_count,
                                           const int sink_count,
                                           const int completed_node_count,
                                           const ProcessResult& result) {
    ProcessEvent event;
    event.name = "graph.node.failed";
    event.subject = node_name;
    event.related_subject = "graph";
    event.pid = result.pid;
    event.exit_code = result.exit_code;
    event.signal_number = result.signal_number;
    event.terminal_cause = result.signaled ? "signal" : "exit_non_zero";
    event.stream_name = result.stderr_data.empty() ? "stdout" : "stderr";
    event.byte_count = static_cast<int>(result.stderr_data.empty() ? result.stdout_data.size() : result.stderr_data.size());
    event.step_count = 0;
    event.completed_step_count = 0;
    event.node_count = node_count;
    event.sink_count = sink_count;
    event.completed_node_count = completed_node_count;
    event.stdout_excerpt = truncate_excerpt(result.stdout_data);
    event.stderr_excerpt = trim_trailing_newlines(result.stderr_data);
    return event;
}

std::string format_process_failure(const std::string& subject,
                                   const ProcessSpec& spec,
                                   const ProcessResult& result) {
    std::ostringstream message;
    message << subject << " failed";
    if (result.exited) {
        message << " with exit code " << result.exit_code;
    } else if (result.signaled) {
        message << " with signal " << result.signal_number;
    } else {
        message << " with unknown termination state";
    }
    message << ":" << render_command(spec);

    const auto trimmed_stderr = trim_trailing_newlines(result.stderr_data);
    if (!trimmed_stderr.empty()) {
        message << " | stderr: " << trimmed_stderr;
    }
    return message.str();
}

std::string render_process_event_json(const ProcessEvent& event) {
    std::ostringstream out;
    out << '{'
        << "\"name\":\"" << json_escape(event.name) << "\","
        << "\"subject\":\"" << json_escape(event.subject) << "\","
        << "\"related_subject\":\"" << json_escape(event.related_subject) << "\"," 
        << "\"pid\":" << event.pid << ','
        << "\"exit_code\":" << event.exit_code << ','
        << "\"signal_number\":" << event.signal_number << ','
        << "\"terminal_cause\":\"" << json_escape(event.terminal_cause) << "\","
        << "\"stream_name\":\"" << json_escape(event.stream_name) << "\","
        << "\"byte_count\":" << event.byte_count << ','
        << "\"step_count\":" << event.step_count << ','
        << "\"completed_step_count\":" << event.completed_step_count << ','
        << "\"node_count\":" << event.node_count << ','
        << "\"sink_count\":" << event.sink_count << ','
        << "\"completed_node_count\":" << event.completed_node_count << ','
        << "\"stdout_excerpt\":\"" << json_escape(event.stdout_excerpt) << "\","
        << "\"stderr_excerpt\":\"" << json_escape(event.stderr_excerpt) << "\""
        << '}';
    return out.str();
}

}  // namespace exec_graph::runtime
