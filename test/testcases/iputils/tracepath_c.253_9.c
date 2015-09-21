//
// Created by ice-phoenix on 9/21/15.
//

#include <errno.h>

int main(int argc, char** argv) {
    errno = argc;
    return 0;
}
