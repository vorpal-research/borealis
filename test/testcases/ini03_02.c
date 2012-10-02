//
// Разыменование неинициализированного или нулевого указателя
//
#include <stdlib.h>

int main(void)
{
    int *p;
    p = NULL;
    return *p; // дефект INI-03
}
