#include "defines.h"
#include <string.h>

#define SIZE 10

// @requires x != 0
int foo(int* x) {
   *x = __NONDET__();
   return __NONDET__();
}

// @requires \arg0 != 0 // crashes LLVM
int bar(int* x);

int main() {
   int i, j, ret, offset, tmp_cnt, tel_data, klen;
   /* source snippet */
   int x[SIZE];

   for (i = 0; i < SIZE; ++i)
      x[i] = __NONDET__();
   
   for (i = 0; i < SIZE; ++i) {
      ret = __NONDET__();
      if (ret != 0)
         return -1;
      tmp_cnt = __NONDET__();
      if (tmp_cnt < 0)
         return -1;
      
      for (offset = 0; offset < tmp_cnt; offset++) {
         ret = foo(&tel_data);
         if ( ret == 0 || ret == 1 )
            return 1;
         else if ( ret == -1 )
            continue;
         
         for (j = 0; x[j] != 0; j++) {
            if (x[i] == 1) {
               memmove(&x[i], &x[i + 1], SIZE - i);
            }
         }
            
         ret = bar(x);

         if (ret != -1) {
            continue;
         }
         
         klen = strlen((const char*)x);
         
         if (klen > 20) {
            x[i] = 0;
         } else if (klen > 0) {
            x[i] = -1;
         }
      }
   }

   return 1;
}
