#include "defines.h"

int a[1000];

// @requires x >= 0
// @requires x < 1000
int foo(int x) {
   int i;

   if (x == 0) return 1;
   ASSERT(x > 0);
   ASSERT(x < 1000);

   for (i = 0; i < x; ++i)
      a[i]=0;
   return 0;
}

int main() {
   int y, z;

   for (y=0; y < 100; ++y)
      foo(y);

   for (z=0; z < 1000; ++z)
      foo(z);

   return 1;
}
