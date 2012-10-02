//
// Разыменование неинициализированного или нулевого указателя
//
#include <stdlib.h>

int main(void)
{
    int *p;
    
    p = malloc(sizeof(int));
    
    if (*p > 0) // дефект INI-01 (гарантированный => обрубание)
    {
        free(p);
    }
    
    return *p;  // дефект INI-01,
                //        INI-03,
                //        RES-01
}
