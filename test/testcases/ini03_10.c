#include <stdlib.h>

// Hi, i am a comment!

// Yep.

#include "test_include.h"

/* @ensures x+x == 23
   @requires 2
 */
   // @ignore
// @requires 2+2


int main(void)
{
    // @requires a != 0
    int a[5] = {0, 1, 2, 3, 4};
    static int *p;
    static int *q;
    int *s = p;
    int *t = q;
    // @unroll 0x100
    // @requires *(*(x+4)) != 0
    // @mask buf0, asd0, as
    if (s == NULL && t == NULL)
    {
	s = &a[0];
	t = &a[4];
	return *s + *t;
    }
    *s = 1;
    *t = 2;
    return 0;
}