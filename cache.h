#ifndef CACHE
#define CACHE

#include <time.h>

#include "log.h"
#include "message.h"

// Struct for each item in the cache
typedef struct item Cache_Item;

struct item {
    Message *msg;
    time_t expiry;

    Cache_Item *next_item;
};

// Struct for cache - a linked list
typedef struct {
    Cache_Item *item;
    unsigned int item_count;
} Cache;

// Creates an empty cache
Cache *create_cache();

// Creates an item containing for the cache
Cache_Item *create_item(Message *msg);

// Adds a message into the cache
void cache_item(Cache *cache, Message *msg);

// Pops the first item from the cache
void pop_cache(Cache *cache);

// Searches the cache for a message
Message *lookup(Cache *cache, Message *msg);

// Checks if item is expired
int check_expired(Cache_Item *item);

// Checks if qn1 and qn2 are the same
int check_qn(Question *qn1, Question *qn2);

// Checks if the form of the questions (num./size qname, qtype, qclass) match
int check_qnform(Question *qn1, Question *qn2);

// Checks if the names of the questions match
int check_qname(Question *qn1, Question *qn2);

// Frees memory allocated for the cache
void free_cache(Cache *cache);

// Frees memory allocated for the cache item
void free_item(Cache_Item *item);

#endif
