//
// Created by ice-phoenix on 9/18/15.
//

#include "defines.h"

#include <stdlib.h>

int main(int argc, char** argv) {
    int base_port = argc;
    char* p;

    ASSUME(argv);
    ASSUME(argv[0]);

    if ( ! base_port) {
        p = strchr(argv[0], '/');
        if (p) {
            *p = 0;
            base_port = atoi(p+1);
        } else
            base_port = 44444;
    }

    return base_port;
}
