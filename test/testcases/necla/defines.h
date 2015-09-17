#include <stdlib.h>

void borealis_assert(unsigned long long);
void borealis_assume(unsigned long long);
int borealis_nondet();

// this is generally fucked up
#define main __main

#define ASSERT(X) borealis_assert((unsigned long long)(X))
#define ASSUME(X) borealis_assume((unsigned long long)(X))
#define __NONDET__ borealis_nondet
