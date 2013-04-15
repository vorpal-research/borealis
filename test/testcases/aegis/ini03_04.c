#include <stdlib.h>

int main(void) {
    int *p;
    
    p = malloc(sizeof(int));
    
    if (*p > 0) {
        free(p);
    }
    
    return *p;
}
