/*
 * runner.h
 *
 *  Created on: Apr 12, 2013
 *      Author: ice-phoenix
 */

#ifndef RUNNER_H_
#define RUNNER_H_

#include <string>
#include <vector>

namespace borealis {

constexpr int E_CHILD_ERROR =  0x00FF;

constexpr int E_UNKNOWN     = -0x0001;
constexpr int E_CANNOT_FORK = -0x0002;
constexpr int E_EXEC_ERROR  = -0x0003;
constexpr int E_COREDUMP    = -0x0004;

class Runner {

public:

    Runner(const std::string& program);
    Runner& withArg(const std::string& arg);
    int run();

private:

    std::string program;
    std::vector<std::string> args;

};

} /* namespace borealis */

#endif /* RUNNER_H_ */
