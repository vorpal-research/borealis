#include <stdlib.h>
#include <string.h>

const char * keywords[] = {"a", "b", "c", "d", 0};

void defect1(void);
void defect2(void);
void defect3(void);

int main(int argc, char *argv[]) {
    // @assume argv != 0

    const char **ps = keywords;
    int shift;
    while (*ps && strcmp(*ps, *argv)) {
        ps++;
    }

    shift = ps - keywords;
    switch(shift) {
    case 0: defect1();
    case 1: defect2();
    case 2: defect3();
    }

    return 0;
}

void defect1(void) {
    int *p = NULL;
    *p = 1;
    exit(1);
}

void defect2(void) {
    int *q = NULL;
    *q = 1;
    exit(2);
}

void defect3(void) {
    int *r = NULL;
    *r = 1;
    exit(3);
}
