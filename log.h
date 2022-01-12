#ifndef LOG
#define LOG

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>

#include "message.h"

// Logs the request line
void log_request(Message *msg);

// Logs the result line
void log_result(Message *msg);

// Logs an unimplemented request
void log_unimplemented();

// Logs if found has been found in cache
void log_found(Message *msg, time_t expiry);

// Logs cache eviction
void log_replace(Message *prev, Message *next);

// Gets the current time or expiry as string
char *get_time_str(time_t expiry);

#endif
