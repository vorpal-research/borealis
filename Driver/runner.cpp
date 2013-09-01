/*
 * runner.cpp
 *
 *  Created on: Apr 12, 2013
 *      Author: ice-phoenix
 */

#include <sys/wait.h>
#include <unistd.h>

#include "Driver/runner.h"

namespace borealis {
namespace driver {

Runner::Runner(const std::string& program) : program(program) {
    this->args.push_back(program);
}

Runner& Runner::withArg(const std::string& arg) {
    this->args.push_back(arg);
    return *this;
}

Runner& Runner::withArgs(const std::vector<std::string>& args) {
    this->args.insert(this->args.end(), args.begin(), args.end());
    return *this;
}

int Runner::run() {

    int pid = fork();

    if (pid == -1) return E_CANNOT_FORK;
    if (pid == 0) {
        std::vector<char*> execv_args;
        execv_args.reserve(args.size() + 1);
        for (const auto& arg : args) {
            execv_args.push_back(const_cast<char*>(arg.c_str()));
        }
        execv_args.push_back(nullptr);

        const std::vector<char*> const_execv_args(std::move(execv_args));

        int res = execv(const_execv_args[0], &const_execv_args[0]);
        if (res == -1) exit(E_CHILD_ERROR);
        else exit(res);
    }

    int childExitStatus = 0;
    wait(&childExitStatus);

    if (WIFEXITED(childExitStatus)) {
        int res = WEXITSTATUS(childExitStatus);
        if (res == E_CHILD_ERROR) {
            return E_EXEC_ERROR;
        } else {
            return res;
        }
    }

    if (WIFSIGNALED(childExitStatus) && WCOREDUMP(childExitStatus)) {
        return E_COREDUMP;
    }

    return E_UNKNOWN;
}

} // namespace driver
} // namespace borealis
