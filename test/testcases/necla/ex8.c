#include "defines.h"

int main() {
   int* a;
   int k = __NONDET__();
   int i;
   if (k <= 0) return -1;
   
   a = malloc(k * sizeof(int));
   ASSUME(a);

   for (i = 0; i != k; i++)
      if (a[i])
      	return 1;

   return 0;
}
