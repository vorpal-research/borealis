#include "defines.h"

int check(int* x) {
   if (!x) return 0;
   return 10;
}

// @requires \is_valid_ptr(a)
// @requires \is_valid_ptr(b)
int copy(int* a, int* b) {
   *a = *b;
   return 2;
}

// @requires \is_valid_ptr(a) && \bound(a) > n || a == \nullptr
// @requires \is_valid_ptr(b) && \bound(b) > n || b == \nullptr
int foo(int* a, int* b, int n) {
   int i;

   if (n <= 0) return 1;
   if (!check(a)) return 1;
   if (!check(b)) return 1;

   // @assume a != 0
   // @assume b != 0

   for (i=0; i < n; ++i) {
      copy(a+i,b+i);
   }
   copy(a+n,b+n);

   return 1;
}

int main() {
   int a[100], b[200];

   foo(NULL, a,    100);
   foo(a,    NULL, 200);
   foo(a,    b,    50);
   foo(a,    b,    20);
   foo(a,    b,    200);

   return 2;
}
