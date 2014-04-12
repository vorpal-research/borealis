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

// int* my_malloc(size_t size) {
//     int* res = malloc(sizeof(int) * size);
//     if (res) 
//         return res;
//     else 
//         exit(255);
// }

int main() {

    int* arr = my_malloc(16, 1);
    // int* arr = my_malloc(16);

    for (int i = 0; i < 16; ++i) {
        arr[i] = i;
    }

    return arr[15];
}
