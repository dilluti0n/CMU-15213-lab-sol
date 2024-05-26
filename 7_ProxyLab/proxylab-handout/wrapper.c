#include "csapp.h"
#include <asm-generic/errno.h>
#include "wrapper.h"

extern int verbose;

int wrap_open_listenfd(char *port)
{
    int fd;
    if ((fd = open_listenfd(port)) < 0) {
        ERR_MSG("cannot start server on port %s", port);
        exit(1);
    }
    NORMAL_MSG("server listening on port %s", port);
    return fd;
}

int wrap_open_clientfd(char *hostname, char *port)
{
    int fd;

    fd = open_clientfd(hostname, port);
    if (fd > 0) {
        VERBOSE_MSG("%s:%s connected on fd%d", hostname, port, fd);
    }
    return fd;
}

ssize_t wrap_rio_readlineb(rio_t *rp, void *usrbuf, size_t maxlen)
{
    ssize_t rc;
    while ((rc = rio_readlineb(rp, usrbuf, maxlen)) < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            perror("proxy: read");
            continue;
        }
        perror("err: read");
        return -1;
    }
    VERBOSE_MSG("fd%d> %s", rp->rio_fd, (char *)usrbuf);
    return rc;
}

int wrap_accept(int s, struct sockaddr *addr, socklen_t *addrlen)
{
    int rc;

    while ((rc = accept(s, addr, addrlen)) < 0) {
        if (errno == EAGAIN || errno == ENETDOWN || errno == EPROTO ||
            errno == ENOPROTOOPT || errno == EHOSTDOWN || errno == ENONET ||
            errno == EHOSTUNREACH || errno == EOPNOTSUPP ||
            errno == ENETUNREACH) {
            perror("proxy: accept");
            continue;
        }
        perror("err: accept");
        return -1;
    }
    return rc;
}

void wrap_close(int fd)
{
    if (close(fd) < 0) {
        ERR_MSG("fatal: close(%d): %s", fd, strerror(errno))
        exit(2);
    }
    VERBOSE_MSG("fd%d closed.", fd);
}
