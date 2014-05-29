#include <unistd.h>

#include "dbglog.h"

#include <pthread.h>
#include <signal.h>
#include <sys/types.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

void* thread1(void *name) {
    int i;
    logAppName((const char*) name);
    for (i = 0; i < 100; i++) {
        logAppName((const char*) name);
        LOG(INFO1, "ahoj, jak se mas? %d + %d = %d :-)", 1, 1, 1 + 1);
        LOG(INFO3, "baf! %s", "he he he :-P");
        LOG(FATAL4, "F4");

        LOG(FATAL3, "F3");
        LOG(FATAL2, "F2");
        logAppName(PID_TID_APPNAME);
        LOG(FATAL1, "F1");
    }
    return NULL;
}

void* thread2(void *name) {
    int i;

    const char *names[] = {"[prvni]", "[druhy]", "[treti]"};

    logAppName((const char*) name);
    for (i = 0; i < 100; i++) {
        logAppName(names[i % 3]);
        LOG(INFO3, "HUH (%d)", getpid());
        logAppName(PID_APPNAME);
        LOG(INFO3, "PID HUH (%d)", getpid());
        logAppName(PID_TID_APPNAME);
        LOG(INFO3, "PID TID HUH (%d:%lu)", getpid(), pthread_self());
        logAppName((const char*) name);
        LOG(INFO2, "Bleeeee");
        LOG(INFO1, "baf!!!");
    }
    return NULL;
}

//#define DBGLOG_USE_THREADS
//#define DBGLOG_USE_PROCESSES

void hup(int sig) {
    printf("HUP: %d\n", getpid());
    return;
}

void process() {
    for (;;) {
        LOG(INFO4, "ja jsem nakej proces %d", getpid());
    }
}

int main(int argc, char *argv[]) {
#ifdef DBGLOG_USE_THREADS
    pthread_t th1;
    pthread_t th2;
    pthread_t th11;
    pthread_t th21;
    void *ret;
#endif
    logFile("/dev/stdout");

#if defined(DBGLOG_USE_THREADS)

    pthread_create(&th1, NULL, thread1, (void*) "[thread1]");
    pthread_create(&th2, NULL, thread2, (void*) "[thread2]");

    pthread_create(&th11, NULL, thread1, (void*) "[thread3]");
    pthread_create(&th21, NULL, thread2, (void*) "[thread4]");

    pthread_join(th1, &ret);
    pthread_join(th2, &ret);
    pthread_join(th11, &ret);
    pthread_join(th21, &ret);
#elif defined(DBGLOG_USE_PROCESSES)
#define NR 10
/*     { */
/*         int i; */
/*         signal(SIGHUP, hup); */
/*         for (i = NR; i; --i) { */
/*             pid_t pid = fork(); */
/*             if (!pid) process(); */
/*         } */
/*         for (;;)  kill(-getpid(), SIGHUP); */
/*     } */
    {
        pid_t logger, sleeper;
        int i;
        struct sigaction sa;
        sa.sa_handler = hup;
        sa.sa_flags = 0;

        sigemptyset(&sa.sa_mask);
        sigaddset(&sa.sa_mask, SIGHUP);
        sigaction(SIGHUP, &sa, NULL);

        logger = fork();
        if (!logger) {
            LOG(INFO4, "ja jsem nakej logger %d", getpid());
            return 0;
        }
        sleep(1);
        sleeper = fork();
        if (!sleeper) {
            LOG(INFO4, "ja jsem nakej sleeper %d", getpid());
            return 0;
        }
        sleep(1);
        for (i = 20; i; i--) {
            usleep(10000);
            kill(sleeper, SIGHUP);
        }
        sleep(12);
        return 0;
    }
#else
    thread1((void*) "[app1]");
    thread2((void*) "[app2]");
#endif

    LOG(FATAL4,"nnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnn");

    exit(0);
    /* return 0; */
}
