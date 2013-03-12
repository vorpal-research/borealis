//
// Разыменование неинициализированного или нулевого указателя
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct el {
    double d;
    char c;
};

// @ensures \res <= 0
int main() {
    struct el *struct_arr_p[10];
    char ch = '0';
                double tmp = 0.0;
    int i;

    for (i = 0; i < 10; i++) {
        struct_arr_p[i] = (struct el *)malloc(sizeof(struct el));
        if (!struct_arr_p[i]) {
            exit(-1);
        }

        scanf(" %lf %c", &tmp, &ch);

        struct_arr_p[i]->d = tmp;
        struct_arr_p[i]->c = ch;
    }

    free(struct_arr_p[5]);

    for (i = 0; i < 10; i++) {
        printf("Elements: %lf %c", 	// дефект INI-03 на шестой
            struct_arr_p[i]->d,     // итерации цикла
            struct_arr_p[i]->c);	//
    }

    for (i = 0; i < 10; i++) {
        if (i == 5) {
            continue;
        }

        free(struct_arr_p[i]);
    }

    return 0;
}
