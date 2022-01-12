#ifndef MESSAGE
#define MESSAGE

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>

// Header struct for DNS query/response
typedef struct {
    u_int16_t id;
    u_int16_t flgs;
    u_int16_t qns;
    u_int16_t ans_rr;
    u_int16_t athr_rr;
    u_int16_t add_rr;
} Header;

// Name struct for each section of the name in a question
typedef struct {
    u_int8_t len;
    unsigned char *label;

    u_int8_t label_len;
} Name_Section;

// Question struct for DNS query/response
typedef struct {
    Name_Section **name;
    int name_count;
    int name_size;

    u_int16_t qtype;
    u_int16_t qclass;
} Question;

// Answer struct for DNS response
typedef struct {
    u_int16_t name;
    u_int16_t type;
    u_int16_t rrclass;
    u_int32_t ttl;

    u_int16_t rd_len;
    unsigned char *rdata;

    int rdata_len;
} Answer;

// Message struct for DNS query/response
typedef struct {
    u_int16_t tcp_hdr;
    int size;

    Header *hdr;
    Question **qn_list;
    Answer **ans_list;

    int qn_count;
    int ans_count;

    unsigned char *add;
    int add_len;
} Message;

// Stores a query/response in a struct
Message *create_msg(unsigned char *buffer, int size);

// Reads and stores header of dns message
Header *create_hdr(unsigned char *buffer, int *pos);

// Reads and stores questions in the dns message
Question **create_qn_list(unsigned char *buffer, int *pos, int count);

// Gets the entire domain name specified in the qname
void get_qname(Question *qn, unsigned char *buffer, int *pos);

// Creates section of the name in a question
Name_Section *create_section(unsigned char *buffer, int *pos);

// Reads and stores answers in the dns message
Answer **create_ans_list(unsigned char *buffer, int *pos, int count);

// Extracts the domain name from the raw data
char *get_domain(Message *msg);

// Reads one byte from the buffer
u_int8_t get_one_byte(unsigned char *buffer, int *pos);

// Reads two bytes from the buffer
u_int16_t get_two_bytes(unsigned char *buffer, int *pos);

// Reads four bytes from the buffer
u_int32_t get_four_bytes(unsigned char *buffer, int *pos);

// Reads n bytes from the buffer
unsigned char *get_n_bytes(unsigned char *buffer, int *pos, int n);

// Frees memory allocated for the message
void free_msg(Message *msg);

// Frees memory allocated for the message questions
void free_qn(Question **qn_list, int qn_count);

// Frees memory allocated for the message question labels
void free_qname(Question *qn);

// Frees memory allocated for the message answers
void free_ans(Answer **ans_list, int ans_count);

#endif
