/* wrapper.h - wrapper module for functions on csapp.c */
#ifndef WRAPPER_H_
#define WRAPPER_H_

#include "csapp.h"

#define ERR_MSG(format, ...) \
fprintf(stderr, "proxy: err: " format "\n", ##__VA_ARGS__);

#define NORMAL_MSG(format, ...) \
printf("proxy: " format "\n", ##__VA_ARGS__);

#define VERBOSE_MSG(format, ...) \
if (proxy_verbose) {\
    printf("proxy: " format "\n", ##__VA_ARGS__);\
}

int wrap_open_listenfd(char *port);
int wrap_open_clientfd(char *hostname, char *port);
ssize_t wrap_rio_writen(int fd, void *usrbuf, size_t n);
ssize_t wrap_rio_readlineb(rio_t *rp, void *usrbuf, size_t maxlen);
int wrap_accept(int s, struct sockaddr *addr, socklen_t *addrlen);
void wrap_close(int fd);

#endif /* endof wrapper.h */
