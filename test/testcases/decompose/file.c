#include <stdio.h>
#include <stdlib.h>

int main() {

    FILE* in = fopen("file.txt", "r");
    if(in) {
        fclose(in);
        return fgetc(in);
    } else return 0;
}
