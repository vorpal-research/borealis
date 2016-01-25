/*
 * wrapper.cpp
 *
 * Created on: Nov 1, 2012
 *     Author: belyaev
 */

#include <iostream>

#include <execinfo.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

#include <z3/z3++.h>

#include "Driver/gestalt.h"

void handler(int sig) {
    void *array[30];
    size_t size;

    // get void*'s for all entries on the stack
    size = backtrace(array, 30);

    // print out all the frames to stderr
    std::cerr << "Error: signal %d:\n" << sig << std::endl;
    backtrace_symbols_fd(array, size, STDERR_FILENO);
    exit(1);
}

int main(int argc, const char** argv) {
    signal(SIGSEGV, handler);   // install crash handler
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
