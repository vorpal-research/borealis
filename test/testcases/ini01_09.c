#include <stdio.h>

extern int arr[10];

int getArrayElem(const int* arr, int index)
{
   return arr[index];
}

int main(void)
{
   int index;
   //scanf("%d",&index);
   if (index >= 0)
      return 0;
   // Здесь должен быть только BUF-02
   return getArrayElem(arr, index);
}