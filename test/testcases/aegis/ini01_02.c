#include <stdio.h>

int main(int argc, char *argv[]) {
    int a;
    int i;
    int *q;
    int *r;
    int **p;
    
    r = &a;
    
    a = argc;
    
    if (a > 3) {
        q = &i;
        p = &q;
    } else {
        q = &a;
        p = &r;
    }
    
    return **p;
}
