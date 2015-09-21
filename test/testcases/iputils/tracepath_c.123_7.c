//
// Created by ice-phoenix on 9/21/15.
//

#include <errno.h>
#include <sys/socket.h>

int recverr(int fd) {

    int res;
    struct msghdr msg;

restart:
    res = recvmsg(fd, &msg, MSG_ERRQUEUE);
    if (res < 0) {
        if (errno == EAGAIN)
            return -1;
        goto restart;
    }

    return 0;
}

int main(int argc, char** argv) {
    recverr(argc);
    return 0;
}
