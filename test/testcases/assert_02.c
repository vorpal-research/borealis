#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

struct el {
    double d;
    char c;
};

int main(void) {
    struct el* ptr;
    ptr = (struct el*)malloc(2 * sizeof(struct el));
    // @assert ptr != 0

    ptr[0].c = 1;
    ptr[1].c = 2;

    return rand() * ptr[0].c;
}