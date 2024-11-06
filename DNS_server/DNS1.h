#ifndef DNS1_H
#define DNS1_H
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define MAX_LABELS 32
#define MAX_HINTS 3
#define MAX_RR 100  // Maximum number of resource records
#define MAX_PAYLOAD_SIZE 512

typedef enum { DNS_CLASS_IN = 1 } DNSClass;
typedef enum { RCODE_NO_ERROR = 0 } RCode;
typedef enum { DNS_TYPE_A = 1 } DNSType;

///////////////////////////////STRUCTS///////////////////////////////
//structura pentru reprezentarea unui label DNS
typedef struct {
    char label[256];
} DNSLabel;

//numele DNS-ului care este un array de DNS label-uri
typedef struct {
    DNSLabel labels[MAX_LABELS];
    int label_count;
} DNSName;

//structura pentru ip addres si port
typedef struct {
    char ip[16];
    int port;
} ComboAddress;

//structura pentru hint-uri
typedef struct {
    DNSName dns_name;
    ComboAddress combo_address;
} DNSHint;

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

typedef struct {
    // Fill out the DNS header fields based on requirements
    uint16_t id;
    uint16_t flags;
    uint16_t qdcount;
    uint16_t ancount;
    uint16_t nscount;
    uint16_t arcount;
} DNSHeader;

typedef struct {
    uint8_t data[MAX_PAYLOAD_SIZE];
    uint16_t size;
} DNSPayload;

typedef struct {
    DNSHeader header;
    DNSPayload payload;
    uint16_t payloadpos;
    uint16_t rrpos;
    uint16_t endofrecord;
    uint8_t ednsVersion;
} DNSMessageReader;

typedef struct {
    DNSHeader dh;
    uint8_t payload[MAX_PAYLOAD_SIZE];
    uint16_t payloadpos;
    DNSName qname;
    DNSType qtype;
    DNSClass qclass;
    bool haveEDNS;
    bool doBit;
    bool nocompress;
    RCode ercode;
} DNSMessageWriter;

DNSMessageWriter DNSMessageWriter_init(DNSName* dn, DNSType* dt) {
    DNSMessageWriter dmw;
    memset(&dmw, 0, sizeof(DNSMessageWriter));  // Clear the structure

    dmw.qname = *dn;
    dmw.qtype = *dt;
    dmw.qclass = DNS_CLASS_IN;  // Default to IN class

    return dmw;
}

// Function to initialize a DNS name from a string (e.g., "a.root-servers.net")
DNSName makeDNSName(const char* name) {
    DNSName dns_name;
    dns_name.label_count = 0;

    // Tokenize the name by '.' to split into labels
    char temp[256];
    strncpy(temp, name, sizeof(temp) - 1);
    temp[sizeof(temp) - 1] = '\0';

    char* token = strtok(temp, ".");
    while (token != NULL && dns_name.label_count < MAX_LABELS) {
        strncpy(dns_name.labels[dns_name.label_count++].label, token, 255);
        token = strtok(NULL, ".");
    }
    return dns_name;
}

#endif