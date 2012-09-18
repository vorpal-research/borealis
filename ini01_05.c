//
// INI01 - использование неинициализированной переменной
//
#include<stdio.h>

int main(void)
{
    char c;
    int i;
    float f;
    
    printf("%c %d %f\n", c, i, f); // 3 дефекта INI-01
    
    return 0;
}
