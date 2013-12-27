#include "defines.h"

typedef struct {
   int* a;
   int* b;
   int* c;
} ptrStruct;

int i;

// @requires \is_valid_ptr(ptr)
// @requires \is_valid_ptr(ptr->a)
// @requires \is_valid_ptr(ptr->b)
int foo(ptrStruct* ptr) {
   *(ptr->a) = i++;
   *(ptr->b) = i++;
   return 1;
}

int main() {
   int x,y,z;
   int a,b,c;
   ptrStruct A,B;

   i = 0;

   A.a = &x;
   A.b = &y;
   A.c = &z;
   foo(&A);

   B.a = &a;
   B.b = &b;
   B.c = &c;
   foo(&B);
   
   ASSERT( *(B.b) >= *(A.a) );

   return -1;
}
