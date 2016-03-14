/*
 * wrapper.cpp
 *
 * Created on: Nov 1, 2012
 *     Author: belyaev
 */

#include <iostream>

#define BACKWARD_HAS_UNWIND 1
#define BACKWARD_HAS_BACKTRACE 0
#define BACKWARD_HAS_DW 0
#define BACKWARD_HAS_BFD 0
#define BACKWARD_HAS_BACKTRACE_SYMBOL 1
#include <backward.hpp>

#include <z3/z3++.h>

#include "Driver/gestalt.h"

static backward::SignalHandling sh{std::vector<int>{ SIGABRT, SIGSEGV, SIGILL, SIGINT }};

int main(int argc, const char** argv) {
    using namespace borealis::driver;
    try{
        gestalt gestalt{ "wrapper" };
        return gestalt.main(argc, argv);
    } catch(const std::exception& ex) {
        std::cerr << "Exception caught: " << ex.what() << std::endl;
        exit(1);
    } catch(const z3::exception& ex) {
        std::cerr << "Exception caught: " << ex.msg() << std::endl;
        exit(1);
    } catch(...) {
        std::cerr << "Exception caught " << std::endl;
        exit(1);
    }

}
