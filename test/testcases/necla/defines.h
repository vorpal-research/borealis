void borealis_assert(int);
void borealis_assume(int);
int borealis_nondet();

#define ASSERT borealis_assert
#define ASSUME borealis_assume
#define __NONDET__ borealis_nondet
