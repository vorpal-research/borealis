#include <stdlib.h>

// Hi, i am a comment!

// Yep.

#include "test_include.h"

// @requires 2+2
int main(void)
{
    int a[5] = {0, 1, 2, 3, 4};
    static int *p;
    static int *q;
    int *s = p;
    int *t = q;
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