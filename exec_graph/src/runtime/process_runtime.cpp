#include "exec_graph/runtime/process_runtime.hpp"

#include <cerrno>
#include <cstring>
#include <fstream>
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

std::string read_all(int fd) {
    std::string output;
    char buffer[4096];
    while (true) {
        const ssize_t count = read(fd, buffer, sizeof(buffer));
        if (count == 0) {
            break;
        }
        if (count < 0) {
            throw std::runtime_error("failed to read process stdout");
        }
        output.append(buffer, static_cast<std::size_t>(count));
    }
    return output;
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
    if (pipe(stdin_pipe) != 0 || pipe(stdout_pipe) != 0) {
        throw std::runtime_error("failed to create pipes");
    }

    const pid_t pid = fork();
    if (pid < 0) {
        throw std::runtime_error("fork failed");
    }

    if (pid == 0) {
        dup2(stdin_pipe[0], STDIN_FILENO);
        dup2(stdout_pipe[1], STDOUT_FILENO);
        close(stdin_pipe[0]);
        close(stdin_pipe[1]);
        close(stdout_pipe[0]);
        close(stdout_pipe[1]);

        std::vector<char*> argv;
        argv.reserve(spec.argv.size() + 1);
        for (const auto& arg : spec.argv) {
            argv.push_back(const_cast<char*>(arg.c_str()));
        }
        argv.push_back(nullptr);

        execvp(argv[0], argv.data());
        _exit(127);
    }

    close(stdin_pipe[0]);
    close(stdout_pipe[1]);

    if (!stdin_data.empty()) {
        write_all(stdin_pipe[1], stdin_data);
    }
    close(stdin_pipe[1]);

    const std::string stdout_data = read_all(stdout_pipe[0]);
    close(stdout_pipe[0]);

    int status = 0;
    if (waitpid(pid, &status, 0) < 0) {
        throw std::runtime_error("waitpid failed");
    }

    if (WIFEXITED(status)) {
        return ProcessResult{WEXITSTATUS(status), stdout_data};
    }
    if (WIFSIGNALED(status)) {
        return ProcessResult{128 + WTERMSIG(status), stdout_data};
    }
    return ProcessResult{1, stdout_data};
}

std::string run_workflow(const std::vector<ProcessSpec>& workflow) {
    std::string current_output;
    for (const auto& process : workflow) {
        const auto result = run_process(process, current_output);
        if (result.exit_code != 0) {
            std::ostringstream message;
            message << "command failed with exit code " << result.exit_code << ":";
            for (const auto& arg : process.argv) {
                message << ' ' << arg;
            }
            throw std::runtime_error(message.str());
        }
        current_output = result.stdout_data;
    }
    return current_output;
}

}  // namespace exec_graph::runtime
