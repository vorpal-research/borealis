//
// Created by ice-phoenix on 9/18/15.
//

#include "defines.h"

#include <stdlib.h>
#include <netdb.h>

struct sockaddr_in target;

int main(int argc, char** argv) {
    struct hostent* he;
    char* p;

    ASSUME(argv);
    ASSUME(argv[0]);

    p = argv[0];

    target.sin_family = AF_INET;

    he = gethostbyname2(p, AF_INET);
    if (he == NULL) {
        herror("gethostbyname2");
        exit(1);
    }

    // @assume \is_valid_ptr(he)
    // @assume \is_valid_ptr(he->h_addr_list)
    // @assume \is_valid_ptr(he->h_addr_list[0])

    memcpy(&target.sin_addr, he->h_addr, 4);

    return 0;
}
