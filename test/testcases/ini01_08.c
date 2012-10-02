/*
 * Тест проверяет что при копировании не появляется неинициализированных значений (см. #2715)
 */

#include <string.h>

int main(int argc) {
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

    memmove(pdst, psrc, 5); // Переполнение буфера

    return a[4];
}
