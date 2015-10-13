//
// Created by ice-phoenix on 9/22/15.
//

#include <time.h>

struct timespec start;

// @requires \is_valid_ptr(a)
// @requires \is_valid_ptr(b)
static int timespec_later(struct timespec *a, struct timespec *b)
{
    return (a->tv_sec > b->tv_sec) ||
           ((a->tv_sec == b->tv_sec) && (a->tv_nsec > b->tv_nsec));
}

int main() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    if (ts.tv_sec == 0) {
        start = ts;
    }
    return timespec_later(&start, &ts);
}
