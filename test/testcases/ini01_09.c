#include <stdio.h>



extern int arr[10];

int getArrayElem(const int* arr, int index)
{
   return arr[index];
}

// @requires argc > 0
// @ensures \result <= 0
int main(int argc, char* argv[])
{
   int index;
   scanf("%d",&index);
   if (index >= 0)
      return 0;
   // Здесь должен быть только BUF-02
   return getArrayElem(arr, index);
}