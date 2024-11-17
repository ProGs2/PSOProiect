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

///////////////////////////////STRUCTS///////////////////////////////
//for hints
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
/////////////////////////////////////////////////////////////
//
typedef struct{

}DNSType;
typedef struct{
    DNSName dns_name;
    DNSType dns_type;
    uint32_t ttl;
    void* data;
}ResourceRecord; //ResolveRR

typedef struct{
    ResourceRecord** records;
    size_t record_count;
    size_t capacity;
    uint32_t ttl;
}RRSet; //RRSet + RRGen

typedef struct{
    ResourceRecord** results;
    ResourceRecord** intermadiate;
    size_t results_count;
    size_t intermadiate_count;
}ResolveResult;

typedef struct{
    DNSHint* d_root;
    size_t d_maxqueries;
    bool d_skipIPv6;
    FILE* d_dot;
    FILE* d_log;
    unsigned int d_numqueries;
    unsigned int d_numtimeouts;
    unsigned int d_numformerrs;
}TDNSResolver;



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

DNSMessageReader getResponse(struct ComboAddress* server, struct DNSName* dn, struct DNSType* dt, int depth)
{
    bool doEDNS = true, doTCP = false;
    for(int tries = 0; tries < 4; ++tries)
    {
        DNSMessageWriter dmw = DNSMessageWriter_init(dn, dt);
        
    }
}

#endif