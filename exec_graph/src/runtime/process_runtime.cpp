#include "exec_graph/runtime/process_runtime.hpp"

#include <cerrno>
#include <array>
#include <cstring>
#include <fstream>
#include <poll.h>
#include <sstream>
#include <stdexcept>
#include <system_error>
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
            workflow.push_back(ProcessSpec{std::move(tokens)});
        }
    }

    if (workflow.empty()) {
        throw std::runtime_error("workflow file contained no runnable commands");
    }

    return workflow;
}

ProcessResult run_process(const ProcessSpec& spec, const std::string& stdin_data) {
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

    close(stdin_pipe[0]);
    close(stdout_pipe[1]);
    close(stderr_pipe[1]);

    if (!stdin_data.empty()) {
        write_all(stdin_pipe[1], stdin_data);
    }
    close(stdin_pipe[1]);

    const auto [stdout_data, stderr_data] = read_stdout_and_stderr(stdout_pipe[0], stderr_pipe[0]);

    int status = 0;
    if (waitpid(pid, &status, 0) < 0) {
        throw std::runtime_error("waitpid failed");
    }

    if (WIFEXITED(status)) {
        return ProcessResult{WEXITSTATUS(status), true, false, 0, stdout_data, stderr_data};
    }
    if (WIFSIGNALED(status)) {
        return ProcessResult{128 + WTERMSIG(status), false, true, WTERMSIG(status), stdout_data, stderr_data};
    }
    return ProcessResult{1, false, false, 0, stdout_data, stderr_data};
}

std::string run_workflow(const std::vector<ProcessSpec>& workflow) {
    std::string current_output;
    for (const auto& process : workflow) {
        const auto result = run_process(process, current_output);
        if (result.exit_code != 0) {
            throw std::runtime_error(format_process_failure("command", process, result));
        }
        current_output = result.stdout_data;
    }
    return current_output;
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

}  // namespace exec_graph::runtime
