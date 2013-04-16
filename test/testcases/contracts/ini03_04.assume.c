#include <stdlib.h>

int main(void) {
    int *p;
    
    p = malloc(sizeof(int));
    // @assume p != 0
    
    if (*p > 0) {
        free(p);
    }
    
    return *p;
}
