//
// Created by ice-phoenix on 9/18/15.
//

#include <stdlib.h>
#include <sys/select.h>

void data_wait(int fd)
{
    fd_set fds;
    struct timeval tv;
    FD_ZERO(&fds);
    FD_SET(fd, &fds);
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    select(fd+1, &fds, NULL, NULL, &tv);
}

int main(int argc, char** argv) {
    data_wait(argc);
    return 0;
}
