/*
 * runner.cpp
 *
 *  Created on: Apr 12, 2013
 *      Author: ice-phoenix
 */

#include <unistd.h>

#include "runner.h"

namespace borealis {

Runner::Runner(const std::string& program) : program(program) {
    this->args.push_back(program);
}

Runner& Runner::withArg(const std::string& arg) {
    this->args.push_back(arg);
    return *this;
}

int Runner::run() {
    std::vector<char*> execv_args;
    execv_args.reserve(args.size() + 1);
    for (const auto& arg : args) {
        execv_args.push_back(const_cast<char*>(arg.c_str()));
    }
    execv_args.push_back(nullptr);

    const std::vector<char*> const_execv_args(std::move(execv_args));

    int res = execv(const_execv_args[0], &const_execv_args[0]);
    if (res != -1) {
        return res;
    } else {
        switch (errno) {
        case ENOENT: return E_PROGRAM_NOT_FOUND;
        case EFAULT: return E_FAULT;
        default: return E_UNKNOWN;
        }
    }
}

} /* namespace borealis */
