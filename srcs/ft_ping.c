#include "../incs/ft_ping.h"

void usage_error(void) {
    printf("usage: ./ft_ping [-v] HOST\n");
    exit(1);
}

void error_exit(char *err) {
    printf("ft_ping: %s\n", err);
    exit(1);
}

size_t ft_strlen(char *s) {
    size_t i = 0;
    while (s[i] != '\0') {
        i++;
    }
    return i;
}

size_t ft_strcmp(char *s1, char *s2) {
    if (ft_strlen(s1) != ft_strlen(s2)) {
        return 1;
    }
    for (size_t i = 0; i < ft_strlen(s1); i ++) {
        if (s1[i] != s2[i])
            return 1;
        if (s1[i] == '\0' || s2[i] == '\0')
            break ;
    }
    return 0;
}

void	*ft_memset(void *b, int c, size_t len)
{
	size_t			i;
	unsigned char	*ptr;

	i = 0;
	ptr = (unsigned char *)b;
	while (i < len)
		ptr[i++] = (unsigned char)c;
	return (b);
}

char *get_ip_from_hostname(char *hostname) {
    struct addrinfo hints;
    struct addrinfo *res;
    struct sockaddr_in *sa_in;
    
    ft_memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;

    int status = getaddrinfo(hostname, NULL, &hints, &res);
    if (status < 0) {
        error_exit("getaddrinfo failed");
    }

    sa_in = (struct sockaddr_in *)res->ai_addr;
    char *ip_address = malloc(INET_ADDRSTRLEN*sizeof(char));
    if (ip_address == NULL) {
        freeaddrinfo(res);
        error_exit("malloc failed");
    }

    if (inet_ntop(res->ai_family, &(sa_in->sin_addr),  ip_address, INET_ADDRSTRLEN) == NULL) {
        freeaddrinfo(res);
        free(ip_address);
        error_exit("inet_ntop failed");
    }

    freeaddrinfo(res);
    return ip_address;
}

void arg_handler(t_env *env, int ac, char **av) {
    char *hostname = NULL;
    for (int i = 1; i < ac; i++) {
        if (ft_strcmp(av[i], "-v") == 0) {
            env->verbose = true;
        } else if (hostname == NULL) {
            hostname = av[i];
        } else {
            error_exit("too many arguments");
        }
    }
    env->hostname = hostname;
    return ;
}

void print_stats(t_env *env) {
    (void)env;
    printf("print_stats\n");
    exit(0);
}


void signal_handler(int signal) {
    if (signal == SIGINT) {
        print_stats(&env);
    }
    return ;
}

void open_socket(t_env *env) {
    ft_memset(&(env->hints), 0, sizeof(env->hints));
    env->hints.ai_family = AF_INET;
    env->hints.ai_socktype = SOCK_RAW;
    env->hints.ai_protocol = IPPROTO_ICMP;

    if (getaddrinfo(env->host_dst, NULL, &(env->hints), &(env->res)) < 0) {
        error_exit("get_addr_info: unknown host");
    }
    if ((env->socket_id = socket(env->res->ai_family, env->res->ai_socktype, env->res->ai_protocol)) < 0) {
        error_exit("socket: error when creating the socket");
    }
    int option_value = 1;
    if (setsockopt(env->socket_id, IPPROTO_IP, IP_HDRINCL, &option_value, sizeof(option_value)) < 0) {
        error_exit("setsockopt: error when setting up the socket's options");
    }
    return ;
}

void print_general_data(t_env *env) {
    (void)env;
    printf("print_general_data");
    return;
}

short my_checksum(unsigned short *data, int len) {
    unsigned long checksum = 0;

    while (len > 1) {
        checksum += *data++;
        len -= sizeof(unsigned short);
    }
    if (len)
        checksum += *(unsigned char*)data;

    while (checksum >> 16) {
        checksum = (checksum & 0xFFFF) + (checksum >> 16);
    }
    return (short)~checksum;
}

void setup_send(t_env *env) {
    ft_memset(&(env->buffer), 0, sizeof(env->buffer));
    env->ip->ip_v = 4;            // Set the IP version to IPv4
    env->ip->ip_hl = 5;           // Set the header length to 20 bytes (5 words)
    env->ip->ip_tos = 0;          // Type of Service (set to 0 for default)
    env->ip->ip_len = htons(sizeof(env->buffer));
    env->ip->ip_id = 0; // Unique identification (you can choose a suitable value)
    env->ip->ip_off = 0;          // Fragment offset and flags (set to 0 for no fragmentation)
    env->ip->ip_ttl = env->ttl;         // Time to Live (adjust as needed)
    env->ip->ip_p = env->res->ai_protocol;  // Protocol (e.g., ICMP)
    env->ip->ip_sum = 0;          // Set checksum to 0 for now (calculate it later)
    inet_pton(env->res->ai_family, env->host_src, &(env->ip->ip_src.s_addr));
    inet_pton(env->res->ai_family, env->host_dst, &(env->ip->ip_dst.s_addr));

    env->icmp->icmp_type = ICMP_ECHO;     // Set the ICMP message type to Echo Request
    env->icmp->icmp_code = 0;             // Set the code to 0 (no subcode for Echo Request)
    env->icmp->icmp_hun.ih_idseq.icd_id = env->pid;    // Identifier (choose a suitable value)
    env->icmp->icmp_hun.ih_idseq.icd_seq = env->seq;
    env->icmp->icmp_cksum = my_checksum((unsigned short*)(env->icmp), sizeof(env->icmp));         // Set checksum to 0 for now (calculate it later)
}

void setup_receive(t_env *env) {
    ft_memset(&(env->buffer), 0, sizeof(env->buffer));
	env->iov[0].iov_base = env->buffer;
	env->iov[0].iov_len = sizeof(env->buffer);
	env->msg.msg_name = env->res->ai_addr;
	env->msg.msg_namelen = env->res->ai_addrlen;
	env->msg.msg_iov = env->iov;
	env->msg.msg_iovlen = 1;
	env->msg.msg_control = &(env->buffer_control);
	env->msg.msg_controllen = sizeof(env->buffer_control);
	env->msg.msg_flags = 0;
}

void timer(int interval)
{
	struct timeval tv_current;
	struct timeval tv_next;

	if (gettimeofday(&tv_current, NULL) < 0)
		error_exit("Error gettimeofday\n");
	tv_next = tv_current;
	tv_next.tv_sec += interval;
	while (tv_current.tv_sec < tv_next.tv_sec ||
			tv_current.tv_usec < tv_next.tv_usec)
	{
		if (gettimeofday(&tv_current, NULL) < 0)
			error_exit("Error gettimeofday\n");
	}
}

bool receive_packet(t_env *env) {
    setup_receive(env);
    int nb_receive = recvmsg(env->socket_id, &(env->msg), MSG_DONTWAIT);
    if (env->icmp->icmp_hun.ih_idseq.icd_id == env->pid) {
        env->packets_recv++;
        // calculate stats
        printf("got echo back: %lu bytes\n", nb_receive - sizeof(*(env->ip)));
        timer(env->interval);
        env->seq++;
        return (false);
    }
    return (true);
}


void loop(t_env *env) {
    struct timeval beg;
    struct timeval now;
    (void)beg;
    (void)now;

    env->packets_sent = 0;
    env->packets_recv = 0;
    env->seq = 0;
    print_general_data(env);
    while (true) {
        setup_send(env);
        if (sendto(env->socket_id, env->buffer, sizeof(env->buffer), 0, env->res->ai_addr, env->res->ai_addrlen) < 0) {
            error_exit("sendto: could not send");
        }
        env->packets_sent++;
        while (receive_packet(env))
            ;
    }
}

int main(int ac, char **av) {
    if (ac < 2) {
        usage_error();
    }
    if (getuid() != 0) {
        error_exit("should be uid 0");
    }
    arg_handler(&env, ac, av);

    env.pid = getpid();
    signal(SIGINT, signal_handler);
    env.count = 0;
    env.interval = 1;
    env.timeout = 1;
    env.ttl = 64;

    env.host_src = "0.0.0.0"; // us
    env.host_dst = get_ip_from_hostname(env.hostname);
    env.min = DBL_MAX;
    open_socket(&env);
    env.ip = (struct ip *)env.buffer;
    env.icmp = (struct icmp *)(env.ip + 1);
    loop(&env);
    return 0;
}
