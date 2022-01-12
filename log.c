#include "log.h"

#define LOG_NAME "dns_svr.log"  // Name of log
#define TIMESTAMP_LEN 30        // Length of timestamp string

#define CURRENT 0               // Flag to get current time

// Logs the request line
void log_request(Message *msg) {
    FILE *log = fopen(LOG_NAME, "a+");
    char *tm_str = get_time_str(CURRENT);

    fprintf(log, "%s requested %s\n", tm_str, get_domain(msg));
    fflush(log);
    fclose(log);
    free(tm_str);
}

// Logs the result line
void log_result(Message *msg) {
    FILE *log = fopen(LOG_NAME, "a+");
    struct sockaddr_in6 addr;
    char res_ip[INET6_ADDRSTRLEN], *tm_str = get_time_str(CURRENT);

    // Gets the IP of the first answer
    memset(&addr, 0, sizeof(addr));
    inet_ntop(AF_INET6, msg->ans_list[0]->rdata, res_ip, INET6_ADDRSTRLEN);

    fprintf(log, "%s %s is at %s\n", tm_str, get_domain(msg), res_ip);
    fflush(log);
    fclose(log);
    free(tm_str);
}

// Logs an unimplemented request
void log_unimplemented() {
    FILE *log = fopen(LOG_NAME, "a+");
    char *tm_str = get_time_str(CURRENT);

    fprintf(log, "%s unimplemented request\n", tm_str);
    fflush(log);
    fclose(log);
    free(tm_str);
}

// Logs if found has been found in cache
void log_found(Message *msg, time_t expiry) {
    FILE *log = fopen(LOG_NAME, "a+");
    char *curr_tm_str = get_time_str(CURRENT);
    char *exp_tm_str = get_time_str(expiry);

    fprintf(log, "%s %s expires at %s\n",
    curr_tm_str, get_domain(msg), exp_tm_str);
    fflush(log);
    fclose(log);
    free(curr_tm_str);
    free(exp_tm_str);
}

// Logs cache eviction
void log_replace(Message *prev, Message *next) {
    FILE *log = fopen(LOG_NAME, "a+");
    char *tm_str = get_time_str(CURRENT);

    fprintf(log, "%s replacing %s by %s\n",
    tm_str, get_domain(prev), get_domain(next));
    fflush(log);
    fclose(log);
    free(tm_str);
}

// Gets the current time or expiry as string
char *get_time_str(time_t expiry) {
    time_t current;
    struct tm *tm_t = malloc(sizeof(*tm_t));
    char *str = malloc(TIMESTAMP_LEN);

    if (expiry) {
        tm_t = localtime_r(&expiry, tm_t);
    } else {
        time(&current);
        tm_t = localtime_r(&current, tm_t);
    }

    strftime(str, TIMESTAMP_LEN, "%FT%T%z", tm_t);
    free(tm_t);
    return str;
}
