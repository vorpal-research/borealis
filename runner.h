/*
 * runner.h
 *
 *  Created on: Apr 12, 2013
 *      Author: ice-phoenix
 */

#ifndef RUNNER_H_
#define RUNNER_H_

#include <algorithm>
#include <list>

namespace borealis {

#define E_CHILD_ERROR        0x00FF

#define E_UNKNOWN           -0x0001
#define E_CANNOT_FORK       -0x0002
#define E_EXEC_ERROR        -0x0003
#define E_COREDUMP          -0x0004

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
