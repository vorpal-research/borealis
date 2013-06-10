#include "defines.h"
#include <stdlib.h>
#include <string.h>

// @requires str != 0
char** parse_token(char* str, char c) {
   int len = strlen(str);
   int loc = 0;
   int num = 0;
   int i;
   char** res;

   for (i = 0; i < len; ++i)
      if (str[i] == c)
         num++;

   res = (char**) malloc(num * sizeof(char*));
   ASSUME(res);

   for (i = 0; i < len; ++i) {
      if (str[i] == c) {
         char* temp_str = (char*) calloc(i-loc+1, 1); // the last char is 0
         memcpy(temp_str, str + loc, i - loc);
         res[num++] = temp_str;
         loc = i + 1;
      }
   }

   return res;
}

int main(char* str, char c) {

   char** tokens;

   ASSUME(str);
   ASSUME(strlen(str) > 0);

   tokens = parse_token(str, c);

   return 1;
}
