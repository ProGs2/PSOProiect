#ifndef DNS_RESOLVER_H
#define DNS_RESOLVER_H

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include "dns-hints.h"

#define MAX_RR 100  // Maximum number of resource records

typedef struct ResolveRR {
    DNSName name;
    uint32_t ttl;
} ResolveRR;

typedef struct ResolveResult {
    ResolveRR res[MAX_RR];
    int res_count;
    ResolveRR intermediate[MAX_RR];
    int intermediate_count;
} ResolveResult;

typedef struct TDNSResolver {
    ComboAddress root[MAX_HINTS];
    int root_count;
    unsigned int max_queries;
    bool skip_ipv6;
    unsigned int num_queries;
    unsigned int num_timeouts;
    unsigned int num_formerrs;
    FILE* log_stream;
    FILE* dot_stream;
} TDNSResolver;

//types of DNS
typedef enum{
    A=1,
    MX=15
}DNSType;



#endif
