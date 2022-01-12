#include "server.h"

#define ON 1                // Keeps server on
#define IPv6_PORT 8053      // Port to accept TCP queries from
#define AAAA 28             // IANA assigned value for AAAA record type
#define TCP_HEADER_SIZE 2   // Size of TCP header

#define NONBLOCKING

// Creates a socket for receiving queries
int create_server_socket() {
    int sockfd;
    struct sockaddr_in6 addr;

    memset(&addr, 0, sizeof(addr));
    addr.sin6_family = AF_INET6;
	addr.sin6_addr = in6addr_any;
	addr.sin6_port = htons(IPv6_PORT);

    // Opens a socket
    if ((sockfd = socket(AF_INET6, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Code from the project specification
    int enable = 1;
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable,
        sizeof(int)) < 0) {
		perror("setsockopt");
		exit(EXIT_FAILURE);
	}

    // Binds address to socket
    if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
		exit(EXIT_FAILURE);
	}

    return sockfd;
}

// Creates a socket for forwarding queries and receiving answers
int create_connection_socket(const char *ip, const int port) {
    int sockfd;
    struct sockaddr_in addr;

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(ip);
    addr.sin_port = htons(port);

    // Opens a socket to server
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Connects to server at address using specified socket
    if (connect(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("connect");
        exit(EXIT_FAILURE);
    }

    return sockfd;
}

// Runs miniature DNS server
void run_server(const char *ip, const int port) {
    int sockfd, clt_sockfd, svr_sockfd;
    Properties *prop = malloc(sizeof(*prop));
    Cache *cache = create_cache();
    sockfd = create_server_socket();
    pthread_t thread;

    // Listens and queues incoming queries
    if (listen(sockfd, SOMAXCONN) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    while (ON) {
        if ((clt_sockfd = accept(sockfd, NULL, NULL)) < 0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        svr_sockfd = create_connection_socket(ip, port);

        prop->clt_sockfd = clt_sockfd;
        prop->svr_sockfd = svr_sockfd;
        prop->cache = cache;

        pthread_create(&thread, NULL, process_message, prop);
    }

    close(sockfd);
    free_cache(cache);
    free(prop);
}

// Handles message processing to/from client and server
void *process_message(void *param) {
    Properties *prop = (Properties*)param;
    int clt_sockfd = prop->clt_sockfd, svr_sockfd = prop->svr_sockfd;
    Cache *cache = prop->cache;
    Message *msg = NULL, *match = NULL;

    msg = receive_msg(clt_sockfd);
    log_request(msg);

    match = lookup(cache, msg);

    // Checks if rcode = 4 or if answer not found in cache
    if (!check_rcode(msg) && !match) {

        send_msg(svr_sockfd, msg);
        msg = receive_msg(svr_sockfd);

        // Caches message if answer exists
        if (msg->ans_count > 0) {
            cache_item(cache, msg);
        }

        // Checks if answer exists and first answer type is AAAA
        if (msg->ans_count > 0 && msg->ans_list[0]->type == htons(AAAA)) {
            log_result(msg);
        }
    } else if (match) {
        match->hdr->id = msg->hdr->id;
        msg = match;
        log_result(msg);
    } else {
        log_unimplemented();
    }

    send_msg(clt_sockfd, msg);

    close(svr_sockfd);
    close(clt_sockfd);

    return NULL;
}

// Reads query/response received in the socket
Message *receive_msg(const int sockfd) {
    unsigned char *size_buffer = read_from_sock(sockfd, TCP_HEADER_SIZE);
    unsigned char *msg_buffer;
    Message *msg = NULL;
    int size;

    memcpy(&size, size_buffer, sizeof(int));
    size = (int)ntohs((int)size);

    msg_buffer = read_from_sock(sockfd, size);
    msg = create_msg(msg_buffer, size);

    memcpy(&msg->tcp_hdr, size_buffer, TCP_HEADER_SIZE);

    // If not response (MSB == 1) and question type is not AAAA
    if (!((ntohs(msg->hdr->flgs) >> 0x0f) & 1U) &&
    !(msg->qn_list[0]->qtype == htons(AAAA))) {
        set_rcode(msg);
    }

    free(size_buffer);
    free(msg_buffer);

    return msg;
}

// Transforms message into response with rcode 4
void set_rcode(Message *msg) {
    // Set qr to 1 (MSB = 1)
    msg->hdr->flgs = htons(ntohs(msg->hdr->flgs) | (1U << 0x0f));
    // Set 4 LSB to Rcode 4 (0100 in binary)
    msg->hdr->flgs = htons(ntohs(msg->hdr->flgs) & ~(1U << 0x03));
    msg->hdr->flgs = htons(ntohs(msg->hdr->flgs) | (1U << 0x02));
    msg->hdr->flgs = htons(ntohs(msg->hdr->flgs) & ~(1U << 0x01));
    msg->hdr->flgs = htons(ntohs(msg->hdr->flgs) & ~1U);
    // Set rd to 0
    msg->hdr->flgs = htons(ntohs(msg->hdr->flgs) & ~(1U << 0x08));
}

// Sends query/response through the socket
void send_msg(const int sockfd, Message *msg) {

    write(sockfd, &msg->tcp_hdr, sizeof(msg->tcp_hdr));

    send_hdr(sockfd, msg->hdr);
    send_qn(sockfd, msg->qn_list, msg->qn_count);
    send_ans(sockfd, msg->ans_list, msg->ans_count);

    write(sockfd, msg->add, msg->add_len);

    if (check_rcode(msg)) {
        free_msg(msg);
    }
}

// Sends header through the socket
void send_hdr(const int sockfd, Header *hdr) {
    write(sockfd, &hdr->id, sizeof(hdr->id));
    write(sockfd, &hdr->flgs, sizeof(hdr->flgs));
    write(sockfd, &hdr->qns, sizeof(hdr->qns));
    write(sockfd, &hdr->ans_rr, sizeof(hdr->ans_rr));
    write(sockfd, &hdr->athr_rr, sizeof(hdr->athr_rr));
    write(sockfd, &hdr->add_rr, sizeof(hdr->add_rr));
}

// Sends questions through the socket
void send_qn(const int sockfd, Question **qn_list, int qn_count) {
    int i;

    for (i = 0; i < qn_count; i++) {
        send_qname(sockfd, qn_list[i]);
        write(sockfd, &qn_list[i]->qtype, sizeof(qn_list[i]->qtype));
        write(sockfd, &qn_list[i]->qclass, sizeof(qn_list[i]->qclass));
    }
}

// Sends question name through the socket
void send_qname(const int sockfd, Question *qn) {
    int i;

    for (i = 0; i < qn->name_count; i++) {
        write(sockfd, &qn->name[i]->len, sizeof(qn->name[i]->len));

        // Exclude the zero byte at the end of qname
        if (i < qn->name_count - 1) {
            write(sockfd, qn->name[i]->label, qn->name[i]->label_len);
        }
    }
}

// Sends answers through the socket
void send_ans(const int sockfd, Answer **ans_list, int ans_count) {
    int i;

    for (i = 0; i < ans_count; i++) {
        write(sockfd, &ans_list[i]->name, sizeof(ans_list[i]->name));
        write(sockfd, &ans_list[i]->type, sizeof(ans_list[i]->type));
        write(sockfd, &ans_list[i]->rrclass, sizeof(ans_list[i]->rrclass));
        write(sockfd, &ans_list[i]->ttl, sizeof(ans_list[i]->ttl));
        write(sockfd, &ans_list[i]->rd_len, sizeof(ans_list[i]->rd_len));
        write(sockfd, ans_list[i]->rdata, ans_list[i]->rdata_len);
    }
}

// Checks if rcode is 4
int check_rcode(Message *msg) {
    // Check each bit in rcode
    if (ntohs(msg->hdr->flgs) & (1U << 0x03) ||
    !(ntohs(msg->hdr->flgs) & (1U << 0x02)) ||
    ntohs(msg->hdr->flgs) & (1U << 0x01) ||
    ntohs(msg->hdr->flgs) & 1U) {
        return 0;
    }

    return 1;
}

// Reads the next given number of bytes from the socket and stores in buffer
unsigned char *read_from_sock(const int sockfd, int num_bytes) {
    unsigned char *buffer = malloc(num_bytes);
    int status, bytes_read = 0; // Initialise to empty

    memset(buffer, 0, num_bytes);

    while (bytes_read < num_bytes) {
        status = read(sockfd, buffer + bytes_read, num_bytes - bytes_read);
        if (status == 0) {
            break;
        } else if (status < 0) {
            perror("read");
            exit(EXIT_FAILURE);
        }
        bytes_read += status;
    }

    return buffer;
}
