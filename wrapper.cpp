/*
 * wrapper.cpp
 *
 * Created on: Nov 1, 2012
 *     Author: belyaev
 */

#include "Driver/gestalt.h"

int main(int argc, const char** argv) {
    using namespace borealis::driver;
    gestalt gestalt{ "wrapper" };
    return gestalt.main(argc, argv);
}
