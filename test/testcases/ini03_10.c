#include <stdlib.h>

// Hi, i am a comment!

// Yep.

//#include "test_include.h"

int x = 11;

// @ignore
// @ensures \result > 0
// @requires argc > 0
int main(int argc, char* argv[])
{
    int a[5] = {0, 1, 2, 3, 4};
    static int *p;
    static int *q;
    int *s = p;
    int *t = q;
    int* dyn = malloc(250);
    // @unroll 0x100
    // @mask buf0, asd0, as
    if (argc > 3)
    {
	s = &a[0];
	t = &a[4];
	return -1 * *s + *t + *dyn;
    }
    *s = 1;
    *t = 2;
    return 1;
}