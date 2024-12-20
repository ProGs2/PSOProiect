#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "cache.h"

struct CacheEntry* createCacheEntry()
{
    struct CacheEntry* cache_entry = (struct CacheEntry*)malloc(sizeof(struct CacheEntry));
    cache_entry->domain_name = NULL;
    cache_entry->next = NULL;
    cache_entry->prev = NULL;
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
    unsigned int hash = 0;
    while (*domain_name) {
        hash = (hash << 5) + *domain_name++;
    }
    return hash % MAX_CACHE;
}

struct DNSCache* addCacheEntry(struct DNSCache* cache, struct CacheEntry* cache_entry)
{
    unsigned int hash_index = hash_function(cache_entry->domain_name);
    struct CacheEntry* entry = (struct CacheEntry*)malloc(sizeof(struct CacheEntry));

    while(1)
    {
        cache = DNSCacheCleanUp(cache);
        if(entry != NULL && entry->domain_name != NULL && cache_entry->domain_name != NULL && strcmp(entry->domain_name, cache_entry->domain_name) == 0)
        {
            printf("Entry already in the DNSCache!\n");
            (*cache->buckets[hash_index]).ttl = cache_entry->ttl;
            (*cache->buckets[hash_index]).timestamp = cache_entry->timestamp;
            free(entry);
            return cache;
        }
        else
        {
            printf("Entry inserted succesfuly!\n");
            cache->buckets[hash_index] = cache_entry;

            free(entry);
            return cache;
        }
    }
}

char* lookupDNSCache(struct DNSCache* cache, char* domain_name)
{
    unsigned int hash_index = hash_function(domain_name);
    if(cache->buckets[hash_index] != NULL)
    {
        return cache->buckets[hash_index]->record_value;
    }
    else
    {
        return NULL;
    }
}

struct DNSCache* DNSCacheCleanUp(struct DNSCache* cache)
{
    time_t now = time(NULL);
    for(int i=0;i<MAX_CACHE;i++)
    {
        if(cache->buckets[i] != NULL && cache->buckets[i]->ttl <= now - cache->buckets[i]->timestamp)
        {
            free(cache->buckets[i]);
        }
    }
    return cache;
}