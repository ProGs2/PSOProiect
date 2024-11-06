#ifndef DNS_HUINTS_H
#define DNS_HUINTS_H

#include <stdio.h>
#include <string.h>

#define MAX_LABELS 32
#define MAX_HINTS 3

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