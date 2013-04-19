#include <stdlib.h>

void borealis_assert(int);
void borealis_assume(int);
int borealis_nondet();

// this is generally fucked up
#define main __main

#define ASSERT(X) borealis_assert((int)(X))
#define ASSUME(X) borealis_assume((int)(X))
#define __NONDET__ borealis_nondet
