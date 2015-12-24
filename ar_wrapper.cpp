//
// Created by belyaev on 11/30/15.
//

#include "Driver/ar_facade.h"

int main (int argc, const char* argv[]) {

    using namespace borealis;
    using namespace driver;

    ar_facade facade;
    return facade.main(argc, argv);
}
