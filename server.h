#ifndef SERVER
#define SERVER

#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <assert.h>
#include <time.h>
#include <pthread.h>

#include "message.h"
#include "cache.h"
#include "log.h"

// Holds server properties
typedef struct {
    int clt_sockfd;
    int svr_sockfd;
    Cache *cache;
} Properties;

// Creates a socket for receiving queries
int create_server_socket();

// Creates a socket for forwarding queries and receiving answers
int create_connection_socket(const char *ip, const int port);

// Runs miniature DNS server
void run_server(const char *ip, const int port);

// Handles message processing to/from client and server
void *process_message(void *param);

// Reads query/response received in the socket
Message *receive_msg(const int sockfd);

// Transforms message into response with rcode 4
void set_rcode(Message *msg);

// Sends query/response through the socket
void send_msg(const int sockfd, Message *msg);

// Sends header through the socket
void send_hdr(const int sockfd, Header *hdr);

// Sends questions through the socket
void send_qn(const int sockfd, Question **qn_list, int qn_count);

// Sends question name through the socket
void send_qname(const int sockfd, Question *qn);

// Sends answers through the socket
void send_ans(const int sockfd, Answer **ans_list, int ans_count);

// Checks if rcode is 4
int check_rcode(Message *msg);

// Reads the next given number of bytes from the socket and stores in buffer
unsigned char *read_from_sock(const int sockfd, int num_bytes);

#endif
