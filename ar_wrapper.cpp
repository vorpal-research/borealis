//
// Created by belyaev on 11/30/15.
//

#include "Driver/ar_facade.h"

#define BACKWARD_HAS_UNWIND 1
#define BACKWARD_HAS_BACKTRACE 0
#define BACKWARD_HAS_DW 0
#define BACKWARD_HAS_BFD 0
#define BACKWARD_HAS_BACKTRACE_SYMBOL 1
#include <backward.hpp>

static backward::SignalHandling sh{std::vector<int>{ SIGABRT, SIGSEGV, SIGILL, SIGINT, SIGTRAP }};

int main (int argc, const char* argv[]) {

    using namespace borealis;
    using namespace driver;

    ar_facade facade;
    return facade.main(argc, argv);
}
