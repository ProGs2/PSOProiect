#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "cache.h"

struct CacheEntry* createCacheEntry()
{
    struct CacheEntry* cache_entry = (struct CacheEntry*)malloc(sizeof(struct CacheEntry));
    cache_entry->domain_name = NULL;
    cache_entry->next = NULL;
    cache_entry->record_value = NULL;
    cache_entry->timestamp = 0;
    cache_entry->ttl = 0;

    return cache_entry;
}

struct DNSCache* initializeDNSCache()
{
    struct DNSCache* cache = (struct DNSCache*)malloc(sizeof(struct DNSCache));
    memset(cache->buckets, 0, sizeof(cache->buckets));
    return cache;
} 

unsigned int hash_function(const char* domain_name) {
    unsigned int hash = 5381;
    int c;
    while ((c = *domain_name++)) {
        hash = ((hash << 5) + hash) + c; // hash * 33 + c
    }
    return hash % MAX_CACHE;
}

struct DNSCache* addCacheEntry(struct DNSCache* cache, struct CacheEntry* cache_entry) {
    unsigned int hash_index = hash_function(cache_entry->domain_name);
    CacheEntry* head = cache->buckets[hash_index];

    // Check if domain already exists
    while (head) {
        if (strcmp(head->domain_name, cache_entry->domain_name) == 0) {
            printf("Entry already exists in cache!\n");
            return cache;
        }
        head = head->next;
    }

    // Set timestamp and TTL for the new entry
    cache_entry->timestamp = time(NULL); // Current time
    cache_entry->ttl = TTL_VALUE_CACHE;  // Set default or custom TTL

    // Add new entry to the front of the list
    cache_entry->next = cache->buckets[hash_index];
    cache->buckets[hash_index] = cache_entry;
    printf("Entry inserted successfully: %s -> %s\n", cache_entry->domain_name, cache_entry->record_value);

    return cache;
}

char* lookupDNSCache(struct DNSCache* cache, char* domain_name) {
    printDNSCache(cache);
    // Clean up expired entries first
    cache = DNSCacheCleanUp(cache);

    unsigned int hash_index = hash_function(domain_name);
    CacheEntry* entry = cache->buckets[hash_index];

    while (entry != NULL) {
        if (strcmp(entry->domain_name, domain_name) == 0) {
            // Refresh timestamp
            entry->timestamp = time(NULL);
            printf("Cache hit: %s -> %s\n", entry->domain_name, entry->record_value);
            return entry->record_value;
        }
        entry = entry->next;
    }

    printf("Cache miss for domain: %s\n", domain_name);
    return NULL; // Not found
}

struct DNSCache* DNSCacheCleanUp(struct DNSCache* cache)
{
    printf("Cache cleaner\n");
    time_t now = time(NULL);

    for (int i = 0; i < MAX_CACHE; i++) {
        CacheEntry* entry;
        CacheEntry* previous;
        if(cache->buckets[i] != NULL)
        {
            entry = cache->buckets[i];
            previous = NULL;
        }
        else{entry = NULL;}
        while (entry != NULL) {
            if (now - entry->timestamp >= entry->ttl) {
                printf("Removing expired entry: %s\n", entry->domain_name);

                // Remove the expired entry
                if (previous == NULL) {
                    cache->buckets[i] = entry->next;
                } else {
                    previous->next = entry->next;
                }

                CacheEntry* temp = entry;
                entry = entry->next;

                free(temp->domain_name);
                free(temp->record_value);
                free(temp);
            } else {
                previous = entry;
                entry = entry->next;
            }
        }
    }
    return cache;
}
void printDNSCache(struct DNSCache* cache) {
    time_t now = time(NULL);

    printf("\n--- DNS Cache Entries ---\n");
    for (int i = 0; i < MAX_CACHE; i++) {
        CacheEntry* entry = cache->buckets[i];
        while (entry != NULL) {
            time_t time_left = entry->ttl - (now - entry->timestamp);

            if (time_left > 0) {
                printf("Domain: %s, Record: %s, TTL Remaining: %ld seconds\n",
                       entry->domain_name,
                       entry->record_value,
                       time_left);
            }

            entry = entry->next;
        }
    }
    printf("--------------------------\n");
}

// create new cache entry filled with relevant data
struct CacheEntry* dns_createNewEntry(const char* domain_name, 
                                     const char* ip_address)
{
    struct CacheEntry* cache_entry = (struct CacheEntry*)malloc(sizeof(struct CacheEntry));

    cache_entry->domain_name = (char*)malloc((strlen(domain_name) + 1) * sizeof(char));
    strcpy(cache_entry->domain_name, domain_name);

    cache_entry->record_value = (char*)malloc((strlen(ip_address) + 1) * sizeof(char));
    strcpy(cache_entry->record_value, ip_address);

    // These will be set when the entry is added to the cache list
    cache_entry->next = NULL;
    cache_entry->timestamp = 0;
    cache_entry->ttl = 0;

    return cache_entry;
}