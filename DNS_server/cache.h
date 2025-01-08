#ifndef CACHE_H
#define CACHE_H

#include <time.h>

#define MAX_CACHE 100
#define TTL_VALUE_CACHE 50

typedef struct CacheEntry{
    char* domain_name;
    char* record_value; // the ip address of the domain_name
    time_t ttl;
    time_t timestamp;
    struct CacheEntry* next;
}CacheEntry;

typedef struct DNSCache{
    CacheEntry* buckets[MAX_CACHE];
}DNSCache;

struct CacheEntry* createCacheEntry();
struct DNSCache* initializeDNSCache();
unsigned int hash_function(const char* domain_name);
struct DNSCache* addCacheEntry(struct DNSCache* cache, struct CacheEntry* cache_entry);
char* lookupDNSCache(struct DNSCache* cache, char* domain_name);
struct DNSCache* DNSCacheCleanUp(struct DNSCache* cache);
void printDNSCache(struct DNSCache* cache);

// create new cache entry filled with relevant data
struct CacheEntry* dns_createNewEntry(const char* domain_name, const char* ip_address);

#endif