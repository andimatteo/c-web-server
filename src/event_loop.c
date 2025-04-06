#include "event_loop.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#ifdef USE_KQUEUE

#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>

int create_event_loop() {
    int kq = kqueue();
    if (kq == -1) {
        perror("kqueue");
    }
    return kq;
}

int add_event(int loop_fd, int fd) {
    struct kevent evSet;
    EV_SET(&evSet, fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, NULL);
    if (kevent(loop_fd, &evSet, 1, NULL, 0, NULL) == -1) {
        perror("kevent ADD");
        return -1;
    }
    return 0;
}

int wait_for_events(int loop_fd, int max_events, int timeout, int *fds_out) {
    struct kevent *evList = (struct kevent *)calloc(max_events, sizeof(struct kevent));
    if (!evList) {
        return -1;
    }

    struct timespec ts;
    struct timespec *pts = NULL;
    if (timeout >= 0) {
        ts.tv_sec = timeout / 1000;
        ts.tv_nsec = (timeout % 1000) * 1000000;
        pts = &ts;
    }

    int nevents = kevent(loop_fd, NULL, 0, evList, max_events, pts);
    if (nevents < 0) {
        perror("kevent wait");
        free(evList);
        return -1;
    }

    for (int i = 0; i < nevents; i++) {
        fds_out[i] = (int)evList[i].ident;
    }

    free(evList);
    return nevents;
}

#elif defined(USE_EPOLL)

#include <sys/epoll.h>

int create_event_loop() {
    int epfd = epoll_create1(0);
    if (epfd == -1) {
        perror("epoll_create1");
    }
    return epfd;
}

int add_event(int loop_fd, int fd) {
    struct epoll_event event;
    memset(&event, 0, sizeof(event));
    event.events = EPOLLIN | EPOLLET; // edge-triggered
    event.data.fd = fd;
    if (epoll_ctl(loop_fd, EPOLL_CTL_ADD, fd, &event) < 0) {
        perror("epoll_ctl ADD");
        return -1;
    }
    return 0;
}

int wait_for_events(int loop_fd, int max_events, int timeout, int *fds_out) {
    struct epoll_event *events = (struct epoll_event *)calloc(max_events, sizeof(struct epoll_event));
    if (!events) {
        return -1;
    }

    int nevents = epoll_wait(loop_fd, events, max_events, timeout);
    if (nevents < 0) {
        perror("epoll_wait");
        free(events);
        return -1;
    }

    for (int i = 0; i < nevents; i++) {
        fds_out[i] = events[i].data.fd;
    }

    free(events);
    return nevents;
}

#endif
