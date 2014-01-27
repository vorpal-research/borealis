#include "defines.h"

// //@ensures (\result == 1 && x != 0) || \result == 0
int check(int* x) {
   if (!x) return 0;
   return 1;
}

// //@requires \is_valid_ptr(a)
// //@requires \is_valid_ptr(b)
int copy(int* a, int* b) {
   *a = *b;
   return 1;
}

// //@requires \bound(a) >= n + 1
// //@requires \bound(b) >= n + 1
int foo(int* a, int* b, int n) {
   int i;

   if (n <= 0) return 1;
   if (!check(a)) return 1;
   if (!check(b)) return 1;

   // //@assume a != \invalid
   // //@assume b != \invalid

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

   return 1;
}
