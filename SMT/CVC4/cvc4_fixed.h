//
// Created by belyaev on 6/16/16.
//

#ifndef AURORA_SANDBOX_CVC4_FIXED_H
#define AURORA_SANDBOX_CVC4_FIXED_H

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wtautological-undefined-compare"
#pragma GCC diagnostic ignored "-W#warnings"
#include <cvc4/cvc4.h>
#pragma GCC diagnostic pop
#undef Mutable // CVC4 authors are assholes
#undef ASSERT_TRUE
#undef ASSERT_FALSE

#endif //AURORA_SANDBOX_CVC4_FIXED_H
