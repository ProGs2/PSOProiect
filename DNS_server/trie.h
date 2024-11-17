#ifndef TRIE_H
#define TRIE_H

#include <stdbool.h>
#include <stdlib.h>

#define NR_MAX_CHILDREN 32 

#ifdef __cplusplus
extern "C" {
#endif

typedef struct NSQuerys{
    char* domain1;
    char* domain2;
};
typedef struct SOAMetadata
{
    long long serial_number;
    int refresh_time;
    int retry_time;
    long long expire_time;
    int minimum_ttl;
};
typedef struct DNSRecord{
    char* type; //A, MX, SOA, NS
    char* value; //ip address that is stored
    bool priority; //1 for MX records
    int ttl; //time to live in seconds
};
typedef struct TrieNode{
    char* label;
    struct TrieNode* childrens[NR_MAX_CHILDREN]; //array of children
    struct DNSRecord* records;
    int nr_records;
    struct SOAMetadata* soa;
    struct NSQuerys* ns;
    int  nr_childrens;
};

void error(char* text);
struct TrieNode* createTrieROOT();
char** getArrayOfDomainNames();
struct TrieNode* createBranch(char** domains);
char** extractWordsFromDomain(const char* domain);
int getCharArraySize(char** array);
int getNrBranches();

#ifdef __cplusplus
}
#endif

#endif