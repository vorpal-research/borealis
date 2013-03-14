#include <stdio.h>
#include <stdlib.h>

static int** global_table;
static int GLOBAL_TABLE_SIZE = 8;

int main() {
    global_table = malloc(sizeof(int*) * GLOBAL_TABLE_SIZE);

    for (int i = 0; i < GLOBAL_TABLE_SIZE; ++i) {
        global_table[i] = NULL;
    }

    int res = 0;
    for (int i = 0; i < GLOBAL_TABLE_SIZE; ++i) {
        res += (int)global_table[i];
    }
    return res;
}
