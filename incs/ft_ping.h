#ifndef FT_PING_H
# define FT_PING_H

# include <unistd.h>
# include <stdio.h>
# include <signal.h>
# include <arpa/inet.h>
# include <sys/types.h>
# include <sys/socket.h>
# include <netinet/ip.h>
# include <netinet/ip_icmp.h>
# include <netdb.h>
# include <sys/time.h>
# include <stdbool.h>
# include <stdlib.h>
# include <float.h>
# include <math.h>

typedef struct s_env {
    // unused flag
    bool verbose;

    // names
    char *hostname;
    char *host_dst;
    char *host_src;

    // communication structures
    pid_t pid;
    struct ip *ip;
    struct icmp *icmp;
    struct addrinfo hints;
    struct addrinfo *res;
    char buffer[1000];

    struct iovec iov[1];
    struct msghdr msg;
    char buffer_control[1000];

    // communication data
    unsigned int seq;
    unsigned int count;
    unsigned int interval;
    unsigned int timeout;
    unsigned int ttl;

    // socket
    int socket_id;

    // calculus data
    unsigned int packets_sent;
    unsigned int packets_recv;
    double min;
    double max;
} t_env;

t_env env;

#endif