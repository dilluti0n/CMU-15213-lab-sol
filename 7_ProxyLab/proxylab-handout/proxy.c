#include "csapp.h"

#define DEFAULT_PORT "55556"
#define MAXPORT 6               /* port <= 65535, five digits */
#define VERBOSE_MSG(format, ...) \
if (verbose) {\
    printf("proxy: " format "\n", ##__VA_ARGS__);\
}

int verbose = 1;

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

static const char *user_agent_hdr = "\
User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 \
Firefox/10.0.3\r\n";

int get_request_from_client(int listenfd, char *request, SA *clientaddr);
int request_and_reply(int connfd, char *request);
int parse_request(char *request, char *hostname, char *newrequest, char *port);

int main(int argc, char *argv[])
{
    struct sockaddr_storage  clientaddr;
    char                    *port, request[MAXLINE];
    int                      listenfd;

    listenfd = Open_listenfd(port = (argc > 1? argv[1] : DEFAULT_PORT));
    printf("proxy: proxy server listening on port %s\n", port);
    for (;;) {
        int connfd;

        connfd = get_request_from_client(listenfd, request, (SA *)&clientaddr);
        request_and_reply(connfd, request);

        Close(connfd);

        VERBOSE_MSG("clientfd%d closed, wait for next connection", connfd);
    }
    Close(listenfd);
    return 0;
}

int get_request_from_client(int sockfd, char *request, SA *clientaddr)
{
    socklen_t clientlen;
    rio_t     rp;
    int       connfd;

    clientlen = sizeof(struct sockaddr_storage);
    connfd = Accept(sockfd, clientaddr, &clientlen);
    Rio_readinitb(&rp, connfd);
    Rio_readlineb(&rp, request, MAXLINE);

    VERBOSE_MSG("clientfd%d> %s", connfd, request);

    return connfd;
}

/*
 * request_and_reply - Establish connection to server and request.
 *         Get reply, and then echo it to client(connfd)
 */
int request_and_reply(int connfd, char *request)
{
    int    clientfd;
    char   hostname[MAXLINE], newrequest[MAX_OBJECT_SIZE], port[MAXPORT];
    char   buf[MAX_OBJECT_SIZE];
    rio_t  rp;

    if (parse_request(request, hostname, newrequest, port) < 0) {
        fprintf(stderr, "proxy: parse_request err\n");
        exit(-1);
    }

    VERBOSE_MSG("request_and_reply: \nhostname, %s\nport, %s\nnewrequest, %s",
                hostname, port, newrequest);

    clientfd = Open_clientfd(hostname, port);

    VERBOSE_MSG("request_and_reply: %s:%s connected on fd%d",
                hostname, port, clientfd);

    Rio_writen(clientfd, newrequest, strlen(newrequest));

    VERBOSE_MSG("request_and_reply: %s;fd%d< %s",
                hostname, clientfd, newrequest);

    Rio_readinitb(&rp, clientfd);
    while (Rio_readlineb(&rp, buf, MAX_OBJECT_SIZE) != 0) {
        Rio_writen(connfd, buf, strlen(buf));
    }
    Close(clientfd);

    VERBOSE_MSG("fd%d closed", clientfd);

    return 0;
}

int parse_request(char *request, char *hostname, char *newrequest, char *port)
{
    char method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    sscanf(request, "%s %s %s", method, uri, version);

    /* parse hostname from uri */
    char *p = strstr(uri, "//");
    p = p == NULL? uri : p + 2;

    int c, i;
    for (i = 0;
         (c = *p) != '/' && c != '\0' && c != ':' && i < MAXLINE;
         p++, i++) {
        hostname[i] = c;
    }
    if (c != '/' && c != ':') {
        fprintf(stderr, "proxy: out of range on hostname\n");
        return -1;
    }
    hostname[i] = '\0';

    /* make newrequest */
    char file[MAXLINE];
    if (c == '/') {
        for (i = 0; (c = *p) != '\0' && c != ':' && i < MAXLINE; p++, i++)
            file[i] = c;
        if (i >= MAXLINE) {
            fprintf(stderr, "proxy: out of range on file\n");
            return -1;
        }
        file[i] = '\0';
    } else { /* c == ':' */
        file[0] = '/';
        file[1] = '\0';
    }

    /* Get port if it is provided, if not, set it to default(80) */
    if (c == ':')
        strncpy(port, p + 1, MAXPORT);
    else
        strcpy(port, "80");

    if (snprintf(newrequest, MAX_OBJECT_SIZE,
                 "%s %s %s\r\n"
                 "Host: %s\r\n"
                 "%s"
                 "\r\n",
                 method, file, "HTTP/1.0", hostname, user_agent_hdr) < 0) {
        fprintf(stderr, "proxy: out of range on newrequest\n");
        return -1;
    }

    return 0;
}
