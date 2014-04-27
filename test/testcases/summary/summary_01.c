#include <stdio.h>
#include <stdlib.h>

int* my_malloc(size_t size, int safe) {
    int* res = malloc(sizeof(int) * size);
    if (safe > 0) {
        if (res) return res;
        else exit(255);
    } else {
        return res;
    }
}

void another_malloc(int *arr, size_t size, int safe) {
    int* res = malloc(sizeof(int) * size);
    if (safe > 0) {
        if (res) 
            arr = res;
        else 
            exit(255);
    } else {
        arr = res;
    }
}

void my_free(int* arr) {
    // if (arr != NULL) {
        free(arr);
    // }
}

int main() {

    int* arr = my_malloc(16, 1);
    // int* arr;
    // another_malloc(arr, 16, 1);

    for (int i = 0; i < 16; ++i) {
        arr[i] = i;
    }
    int sixteen = arr[15];
    my_free(arr);

    return sixteen;
}
