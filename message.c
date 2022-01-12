#include "message.h"

// Stores a query/response in a struct
Message *create_msg(unsigned char *buffer, int size) {
    Message *msg = malloc(sizeof(*msg));
    Header *hdr = NULL;
    Question **qn_list = NULL;
    Answer **ans_list = NULL;
    int pos = 0; // Initialise to start of buffer

    assert(msg);
    
    hdr = create_hdr(buffer, &pos);

    memcpy(&msg->qn_count, &hdr->qns, sizeof(int));
    msg->qn_count = (int)ntohs((int)msg->qn_count);
    memcpy(&msg->ans_count, &hdr->ans_rr, sizeof(int));
    msg->ans_count = (int)ntohs((int)msg->ans_count);
    
    qn_list = create_qn_list(buffer, &pos, msg->qn_count);
    ans_list = create_ans_list(buffer, &pos, msg->ans_count);

    msg->add_len = size - pos;
    msg->add = get_n_bytes(buffer, &pos, msg->add_len);

    msg->hdr = hdr;
    msg->qn_list = qn_list;
    msg->ans_list = ans_list;

    return msg;
}

// Reads and stores header of dns message
Header *create_hdr(unsigned char *buffer, int *pos) {
    Header *hdr = malloc(sizeof(*hdr));
    assert(hdr);

    hdr->id = get_two_bytes(buffer, pos);
    hdr->flgs = get_two_bytes(buffer, pos);
    hdr->qns = get_two_bytes(buffer, pos);
    hdr->ans_rr = get_two_bytes(buffer, pos);
    hdr->athr_rr = get_two_bytes(buffer, pos);
    hdr->add_rr = get_two_bytes(buffer, pos);

    return hdr;
}

// Reads and stores questions in the dns message
Question **create_qn_list(unsigned char *buffer, int *pos, int count) {
    Question **qn_list = malloc(count*sizeof(Question*)), *qn = NULL;
    assert(qn_list);
    int i;

    for (i = 0; i < count; i++) {
        qn = malloc(sizeof(*qn));
        qn->name = malloc(sizeof(qn->name));
        assert(qn);
        assert(qn->name);

        get_qname(qn, buffer, pos);
        qn->qtype = get_two_bytes(buffer, pos);
        qn->qclass = get_two_bytes(buffer, pos);

        qn_list[i] = qn;
    }

    return qn_list;
}

// Gets the entire domain name specified in the qname
void get_qname(Question *qn, unsigned char *buffer, int *pos) {
    Name_Section *sec = NULL;

    // Initialise to empty
    qn->name_count = 0;
    qn->name_size = 0;

    do {
        sec = create_section(buffer, pos);
        qn->name_count++;
        qn->name_size += sec->label_len;

        qn->name = realloc(qn->name, qn->name_count*sizeof(Name_Section*));
        qn->name[qn->name_count - 1] = sec;
    } while (sec->label);
}

// Creates section of the name in a question
Name_Section *create_section(unsigned char *buffer, int *pos) {
    Name_Section *sec = malloc(sizeof(*sec));
    assert(sec);

    sec->len = get_one_byte(buffer, pos);
    memcpy(&sec->label_len, (u_int8_t*)&sec->len, sizeof(u_int8_t));

    // Checks if zero byte
    if (sec->label_len == 0) {
        sec->label = NULL;
        return sec;
    }

    sec->label = get_n_bytes(buffer, pos, sec->label_len);

    return sec;
}

// Reads and stores answers in the dns message
Answer **create_ans_list(unsigned char *buffer, int *pos, int count) {
    Answer **ans_list = malloc(count*sizeof(Answer*)), *ans = NULL;
    assert(ans_list);
    int i;

    for (i = 0; i < count; i++) {
        ans = malloc(sizeof(*ans));
        assert(ans);

        ans->name = get_two_bytes(buffer, pos);
        ans->type = get_two_bytes(buffer, pos);
        ans->rrclass = get_two_bytes(buffer, pos);
        ans->ttl = get_four_bytes(buffer, pos);
        ans->rd_len = get_two_bytes(buffer, pos);

        memcpy(&ans->rdata_len, &ans->rd_len, sizeof(int));
        ans->rdata_len = (int)ntohs((int)ans->rdata_len);

        ans->rdata = get_n_bytes(buffer, pos, ans->rdata_len);

        ans_list[i] = ans;
    }

    return ans_list;
}

// Extracts the domain name from the raw data
char *get_domain(Message *msg) {
    char *dmn;
    int i;

    // Only one question in message - domain in first question
    dmn = malloc(msg->qn_list[0]->name_size + msg->qn_list[0]->name_count);
    memset(dmn, 0, msg->qn_list[0]->name_size + msg->qn_list[0]->name_count);
    assert(dmn);

    // Last value in name_count is for zero byte
    for (i = 0; i < msg->qn_list[0]->name_count - 1; i++) {
        strcat(dmn, (char*)msg->qn_list[0]->name[i]->label);

        // Separates each section of the domain name
        if (i < msg->qn_list[0]->name_count - 2) {
            strcat(dmn, ".");
        }
    }

    return dmn;
}

// Reads one byte from the buffer
u_int8_t get_one_byte(unsigned char *buffer, int *pos) {
    u_int8_t result;
    memcpy(&result, &buffer[*pos], sizeof(u_int8_t));
    *pos += 1; // Moves position in buffer up by one
    return result;
}

// Reads two bytes from the buffer
u_int16_t get_two_bytes(unsigned char *buffer, int *pos) {
    u_int16_t result;
    memcpy(&result, &buffer[*pos], sizeof(u_int16_t));
    *pos += 2; // Moves position in buffer up by two
    return result;
}

// Reads four bytes from the buffer
u_int32_t get_four_bytes(unsigned char *buffer, int *pos) {
    u_int32_t result;
    memcpy(&result, &buffer[*pos], sizeof(u_int32_t));
    *pos += 4; // Moves position in buffer up by four
    return result;
}

// Reads n bytes from the buffer
unsigned char *get_n_bytes(unsigned char *buffer, int *pos, int n) {
    // Allocates space for n bytes + null byte
    unsigned char *result = malloc(n + 1);
    memset(result, 0, n + 1);
    assert(result);

    memcpy(result, &buffer[*pos], n);
    *pos += n;

    return result;
}

// Frees memory allocated for a message
void free_msg(Message *msg) {
    free(msg->hdr);
    free_qn(msg->qn_list, msg->qn_count);
    free_ans(msg->ans_list, msg->ans_count);
    free(msg->add);
    free(msg);
}

// Frees memory allocated for the message questions
void free_qn(Question **qn_list, int qn_count) {
    int i;

    for (i = 0; i < qn_count; i++) {
        free_qname(qn_list[i]);
        free(qn_list[i]);
    }

    free(qn_list);
}

// Frees memory allocated for the message question labels
void free_qname(Question *qn) {
    int i;

    // Frees all labels except for zero byte
    for (i = 0; i < qn->name_count - 1; i++) {
        free(qn->name[i]->label);
        free(qn->name[i]);
    }

    free(qn->name);
}

// Frees memory allocated for the message answers
void free_ans(Answer **ans_list, int ans_count) {
    int i;

    for (i = 0; i < ans_count; i++) {
        free(ans_list[i]->rdata);
        free(ans_list[i]);
    }

    free(ans_list);
}
