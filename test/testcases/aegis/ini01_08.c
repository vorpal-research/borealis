#include <string.h>

int main(int argc, char* argv[]) {
    static char a[5];
    char *psrc;
    char *pdst;

    if (argc > 1) {
        psrc = a + 1;
        pdst = a;
    } else {
        psrc = a + 2;
        pdst = a + 1;
    }

    memmove(pdst, psrc, 5);

    return a[4];
}
