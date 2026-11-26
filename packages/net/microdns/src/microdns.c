
#define _POSIX_C_SOURCE 200809L

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <poll.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <signal.h>

#define DEFAULT_ADDR    "0.0.0.0"
#define DEFAULT_PORT    "53"
#define DEFAULT_RESOLV  "/etc/resolv.conf"
#define DEFAULT_TIMEOUT 3
#define MAX_UPSTREAMS   16
#define DNS_MAX         65535

struct upstream {
    struct sockaddr_storage addr;
    socklen_t len;
    int family;
};

struct udp_job {
    int listener;
    struct sockaddr_storage client;
    socklen_t client_len;
    size_t len;
    unsigned char packet[DNS_MAX];
};

static char *listen_addr = DEFAULT_ADDR;
static char *listen_port = DEFAULT_PORT;
static char *resolv_file = DEFAULT_RESOLV;
static int timeout_sec = DEFAULT_TIMEOUT;

static pthread_mutex_t rr_mutex = PTHREAD_MUTEX_INITIALIZER;
static unsigned int rr_counter;

/* Read nameservers from resolv.conf. */
static int
load_upstreams(struct upstream *servers)
{
    FILE *fp;
    char line[512], host[256];
    int count = 0;

    fp = fopen(resolv_file, "r");
    if (!fp) {
        perror("microdns: fopen resolv.conf");
        return 0;
    }

    while (fgets(line, sizeof(line), fp) && count < MAX_UPSTREAMS) {
        struct addrinfo hints, *res = NULL;

        if (sscanf(line, " nameserver %255s", host) != 1 &&
            sscanf(line, "nameserver %255s", host) != 1)
            continue;

        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_DGRAM;
        hints.ai_flags = AI_NUMERICHOST;

        if (getaddrinfo(host, "53", &hints, &res) != 0 || !res)
            continue;

        if (res->ai_addrlen <= sizeof(servers[count].addr)) {
            memcpy(&servers[count].addr, res->ai_addr,
                   res->ai_addrlen);
            servers[count].len = res->ai_addrlen;
            servers[count].family = res->ai_family;
            count++;
        }

        freeaddrinfo(res);
    }

    fclose(fp);
    return count;
}

static unsigned int
next_upstream(int count)
{
    unsigned int n;

    pthread_mutex_lock(&rr_mutex);
    n = rr_counter++ % (unsigned int)count;
    pthread_mutex_unlock(&rr_mutex);

    return n;
}

static int
wait_fd(int fd, short events)
{
    struct pollfd pfd = {
        .fd = fd,
        .events = events
    };

    int rc;

    do {
        rc = poll(&pfd, 1, timeout_sec * 1000);
    } while (rc < 0 && errno == EINTR);

    if (rc <= 0)
        return -1;

    if (pfd.revents & (POLLERR | POLLHUP | POLLNVAL))
        return -1;

    return 0;
}

static int
send_all(int fd, const void *buf, size_t len)
{
    const unsigned char *p = buf;

    while (len) {
        if (wait_fd(fd, POLLOUT) < 0)
            return -1;

        ssize_t n = send(fd, p, len, 0);
        if (n <= 0)
            return -1;

        p += n;
        len -= (size_t)n;
    }

    return 0;
}

static int
recv_all(int fd, void *buf, size_t len)
{
    unsigned char *p = buf;

    while (len) {
        if (wait_fd(fd, POLLIN) < 0)
            return -1;

        ssize_t n = recv(fd, p, len, 0);
        if (n <= 0)
            return -1;

        p += n;
        len -= (size_t)n;
    }

    return 0;
}

/* Connect to TCP upstream with timeout. */
static int
tcp_connect(struct upstream *up)
{
    int fd, flags, err = 0;
    socklen_t errlen = sizeof(err);

    fd = socket(up->family, SOCK_STREAM, 0);
    if (fd < 0)
        return -1;

    flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0 ||
        fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        close(fd);
        return -1;
    }

    if (connect(fd, (struct sockaddr *)&up->addr, up->len) < 0) {
        if (errno != EINPROGRESS || wait_fd(fd, POLLOUT) < 0) {
            close(fd);
            return -1;
        }

        if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &errlen) < 0 ||
            err != 0) {
            close(fd);
            return -1;
        }
    }

    if (fcntl(fd, F_SETFL, flags) < 0) {
        close(fd);
        return -1;
    }

    return fd;
}

/* Forward one DNS request using TCP upstream. */
static int
forward_tcp(struct upstream *servers, int count,
            const unsigned char *query, size_t qlen,
            unsigned char *answer, size_t *alen)
{
    unsigned int start = next_upstream(count);

    for (int i = 0; i < count; i++) {
        struct upstream *up = &servers[(start + i) % count];
        unsigned char hdr[2];
        int fd;

        fd = tcp_connect(up);
        if (fd < 0)
            continue;

        hdr[0] = (unsigned char)(qlen >> 8);
        hdr[1] = (unsigned char)qlen;

        if (send_all(fd, hdr, 2) < 0 ||
            send_all(fd, query, qlen) < 0 ||
            recv_all(fd, hdr, 2) < 0) {
            close(fd);
            continue;
        }

        *alen = ((size_t)hdr[0] << 8) | hdr[1];

        if (*alen < 12 ||
            recv_all(fd, answer, *alen) < 0) {
            close(fd);
            continue;
        }

        close(fd);

        if (answer[0] != query[0] || answer[1] != query[1])
            continue;

        return 0;
    }

    return -1;
}

/* Forward one DNS request using UDP upstream. */
static int
forward_udp(struct upstream *servers, int count,
            const unsigned char *query, size_t qlen,
            unsigned char *answer, size_t *alen)
{
    unsigned int start = next_upstream(count);

    for (int i = 0; i < count; i++) {
        struct upstream *up = &servers[(start + i) % count];
        int fd = socket(up->family, SOCK_DGRAM, 0);

        if (fd < 0)
            continue;

        if (connect(fd, (struct sockaddr *)&up->addr, up->len) < 0 ||
            send(fd, query, qlen, 0) != (ssize_t)qlen ||
            wait_fd(fd, POLLIN) < 0) {
            close(fd);
            continue;
        }

        ssize_t n = recv(fd, answer, DNS_MAX, 0);
        close(fd);

        if (n < 12 ||
            answer[0] != query[0] ||
            answer[1] != query[1])
            continue;

        *alen = (size_t)n;
        return 0;
    }

    return -1;
}

static void *
udp_worker(void *arg)
{
    struct udp_job *job = arg;
    struct upstream servers[MAX_UPSTREAMS];
    unsigned char answer[DNS_MAX];
    size_t alen = 0;
    int count;

    count = load_upstreams(servers);

    if (count > 0 && job->len >= 12 &&
        forward_udp(servers, count, job->packet, job->len,
                    answer, &alen) == 0) {
        sendto(job->listener, answer, alen, 0,
               (struct sockaddr *)&job->client,
               job->client_len);
    }

    free(job);
    return NULL;
}

static void *
tcp_worker(void *arg)
{
    int client = *(int *)arg;
    unsigned char hdr[2];
    unsigned char query[DNS_MAX];
    unsigned char answer[DNS_MAX];

    free(arg);

    for (;;) {
        size_t qlen, alen = 0;
        struct upstream servers[MAX_UPSTREAMS];
        int count;

        if (recv_all(client, hdr, 2) < 0)
            break;

        qlen = ((size_t)hdr[0] << 8) | hdr[1];

        if (qlen < 12 || recv_all(client, query, qlen) < 0)
            break;

        count = load_upstreams(servers);

        if (count == 0 ||
            forward_tcp(servers, count, query, qlen,
                        answer, &alen) < 0)
            break;

        hdr[0] = (unsigned char)(alen >> 8);
        hdr[1] = (unsigned char)alen;

        if (send_all(client, hdr, 2) < 0 ||
            send_all(client, answer, alen) < 0)
            break;
    }

    close(client);
    return NULL;
}

static int
make_listener(int type)
{
    struct addrinfo hints, *res = NULL;
    int fd = -1, one = 1;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = type;
    hints.ai_flags = AI_NUMERICHOST;

    if (getaddrinfo(listen_addr, listen_port, &hints, &res) != 0)
        return -1;

    for (struct addrinfo *ai = res; ai; ai = ai->ai_next) {
        fd = socket(ai->ai_family, type, 0);
        if (fd < 0)
            continue;

        setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

	// listen unisocket
        if (ai->ai_family == AF_INET6) {
            int zero = 0;
	    if (setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY,
	        &zero, sizeof(zero)) < 0) {
                    close(fd);
                    fd = -1;
                    continue;
            }
        }
        //    setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY,
        //               &one, sizeof(one));

        if (bind(fd, ai->ai_addr, ai->ai_addrlen) == 0)
            break;

        close(fd);
        fd = -1;
    }

    freeaddrinfo(res);

    if (fd >= 0 && type == SOCK_STREAM && listen(fd, 128) < 0) {
        close(fd);
        fd = -1;
    }

    return fd;
}

static void
usage(const char *name)
{
    fprintf(stderr,
        "Usage: %s [-a address] [-p port] [-r resolv.conf] [-t timeout]\n",
        name);
    exit(EXIT_FAILURE);
}

int
main(int argc, char **argv)
{
    int opt, udp_fd, tcp_fd;

    while ((opt = getopt(argc, argv, "a:p:r:t:h")) != -1) {
        switch (opt) {
        case 'a': listen_addr = optarg; break;
        case 'p': listen_port = optarg; break;
        case 'r': resolv_file = optarg; break;
        case 't':
            timeout_sec = atoi(optarg);
            if (timeout_sec < 1 || timeout_sec > 60)
                usage(argv[0]);
            break;
        default: usage(argv[0]);
        }
    }

    signal(SIGPIPE, SIG_IGN);

    udp_fd = make_listener(SOCK_DGRAM);
    tcp_fd = make_listener(SOCK_STREAM);

    if (udp_fd < 0 || tcp_fd < 0) {
        perror("microdns: listen");
        return EXIT_FAILURE;
    }

    fprintf(stderr, "microdns: listening on %s:%s\n",
            listen_addr, listen_port);

    for (;;) {
        struct pollfd fds[2] = {
            { .fd = udp_fd, .events = POLLIN },
            { .fd = tcp_fd, .events = POLLIN }
        };

        int rc = poll(fds, 2, -1);
        if (rc < 0) {
            if (errno == EINTR)
                continue;
            perror("microdns: poll");
            break;
        }

        if (fds[0].revents & POLLIN) {
            struct udp_job *job = calloc(1, sizeof(*job));

            if (job) {
                job->listener = udp_fd;
                job->client_len = sizeof(job->client);

                ssize_t n = recvfrom(udp_fd, job->packet,
                                     sizeof(job->packet), 0,
                                     (struct sockaddr *)&job->client,
                                     &job->client_len);

                if (n >= 12) {
                    pthread_t tid;
                    job->len = (size_t)n;

                    if (pthread_create(&tid, NULL, udp_worker, job) == 0)
                        pthread_detach(tid);
                    else
                        free(job);
                } else {
                    free(job);
                }
            }
        }

        if (fds[1].revents & POLLIN) {
            int client = accept(tcp_fd, NULL, NULL);

            if (client >= 0) {
                int *arg = malloc(sizeof(*arg));
                pthread_t tid;

                if (!arg) {
                    close(client);
                    continue;
                }

                *arg = client;

                if (pthread_create(&tid, NULL, tcp_worker, arg) == 0)
                    pthread_detach(tid);
                else {
                    close(client);
                    free(arg);
                }
            }
        }
    }

    close(udp_fd);
    close(tcp_fd);
    return EXIT_FAILURE;
}

