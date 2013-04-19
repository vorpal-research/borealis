#include <stdlib.h>

void borealis_assume(int);

int main(void) {
    int *p;
    
    p = malloc(sizeof(int));
    borealis_assume( p != 0 );
    
    if (*p > 0) {
        free(p);
    }
    
    return *p;
}
