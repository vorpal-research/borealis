#include "defines.h"
#include <stdlib.h>

typedef struct foo{
   int x;
   int z;
} foo_t;

int main(int n) {
   foo_t* a;
   int* b;
   ASSUME(n > 0);
   
   a = (foo_t*) malloc(n * sizeof(foo_t));
   ASSUME(a);

   b = (int*) a; /*-- down casting. Length(b) = 2 * Length(a) --*/
                 /*-- seg faults on clang if `2*n` becomes negative --*/

   b[2*n-1] = '\0';

   return 1;
}
