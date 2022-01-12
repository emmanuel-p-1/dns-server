#include "cache.h"

#define CACHE_LIMIT 5   // Maximum items allowed in cache

// Creates an empty cache
Cache *create_cache() {
    Cache *cache = malloc(sizeof(*cache));
    assert(cache);

    cache->item = NULL;
    cache->item_count = 0; // Initialise to empty

    return cache;
}

// Creates an item containing for the cache
Cache_Item *create_item(Message *msg) {
    Cache_Item *item = malloc(sizeof(*item));
    assert(item);
    time_t current;
    time(&current);

    item->msg = msg;
    // Use ttl of first answer
    item->expiry = current + ntohl(msg->ans_list[0]->ttl);
    item->next_item = NULL;

    return item;
}

// Adds a message into the cache
void cache_item(Cache *cache, Message *msg) {
    Cache_Item *item = cache->item, *new_item = create_item(msg);

    if (!item) {
        // Add item to first pos of cache
        cache->item = new_item;
        cache->item_count++;
        return;
    } else if (check_expired(item)) {
        // Replace first item of cache
        log_replace(cache->item->msg, msg);
        free_item(cache->item);
        cache->item = new_item;
        return;
    }

    while (item) {
        if (item->next_item && check_expired(item->next_item)) {
            // Replace next item if expired
            log_replace(item->next_item->msg, msg);
            new_item->next_item = item->next_item->next_item;
            free_item(item->next_item);
            item->next_item = new_item;
            return;
        } else if (!item->next_item) {
            if (cache->item_count == CACHE_LIMIT) {
                // Replace first item
                log_replace(cache->item->msg, msg);
                pop_cache(cache);
            } else {
                // cache size < 5
                cache->item_count++;
            }
            item->next_item = new_item;
            return;
        }
        item = item->next_item;
    }
}

// Pops the first item from the cache
void pop_cache(Cache *cache) {
    Cache_Item *item = cache->item->next_item;
    free_item(cache->item);
    cache->item = item;
}

// Searches the cache for a message
Message *lookup(Cache *cache, Message *msg) {
    Cache_Item *item = cache->item;
    time_t current;
    time(&current);

    while (item) {
        // Checks if not expired and if first (only one) question match
        if (!check_expired(item) &&
        check_qn(item->msg->qn_list[0], msg->qn_list[0])) {
            // Adjusts the ttl of the first (only one) answer
            item->msg->ans_list[0]->ttl = htonl((unsigned int)difftime(
                item->expiry, current));
            log_found(item->msg, item->expiry);
            return item->msg;
        }
        item = item->next_item;
    }

    return NULL;
}

// Checks if item is expired
int check_expired(Cache_Item *item) {
    time_t current;
    time(&current);

    if (difftime(item->expiry, current) < 0) {
        return 1;
    }
    return 0;
}

// Checks if qn1 and qn2 are the same
int check_qn(Question *qn1, Question *qn2) {

    if (!check_qnform(qn1, qn2) || !check_qname(qn1, qn2)) {
        return 0;
    }

    return 1;
}

// Checks if the form of the questions (num./size qname, qtype, qclass) match
int check_qnform(Question *qn1, Question *qn2) {
    if (qn1->name_count != qn2->name_count ||
        qn1->name_size != qn2->name_size ||
        memcmp(&qn1->qtype, &qn2->qtype, sizeof(u_int16_t)) ||
        memcmp(&qn1->qclass, &qn2->qclass, sizeof(u_int16_t))) {

        return 0;
    }

    return 1;
}

// Checks if the names of the questions match
int check_qname(Question *qn1, Question *qn2) {
    int i;

    for (i = 0; i < qn1->name_count; i++) {
        if (memcmp(qn1->name[i]->label, qn2->name[i]->label,
            qn1->name[i]->label_len)) {
            return 0;
        }
    }

    return 1;
}

// Frees memory allocated for the cache
void free_cache(Cache *cache) {
    Cache_Item *item = cache->item, *next_item = NULL;
    
    while (item) {
        next_item = item->next_item;
        free_item(item);
        item = next_item;
    }

    free(cache);
}

// Frees memory allocated for the cache item
void free_item(Cache_Item *item) {
    free_msg(item->msg);
    free(item);
}
