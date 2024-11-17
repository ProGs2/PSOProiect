#ifndef TDNS_TDNS_H
#define TDNS_TDNS_H

#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>  // For getaddrinfo() and struct addrinfo
#include <arpa/inet.h>  // For inet_ntop()
#include <string.h>

#define zoneFilePath "zone.txt"

#ifdef __cplusplus
extern "C" {
#endif

// Helper function to get the error message
const char* TDNSErrorMessage(int err)
{
    switch(err) {
        case 1: return "DNS lookup failed";
        case 2: return "Timeout error";
        default: return "Unknown error";
    }
}
// Define TDNSContext structure
struct TDNSContext {
    char** servers;  // List of DNS servers to query
    unsigned int timeoutMsec;  // Timeout for queries in milliseconds
};
  
struct TDNSIPAddresses
{
  struct sockaddr_storage** addresses;
  unsigned int ttl;
  void* __handle;
};

struct TDNSMXRecords
{
  struct sockaddr_storage** addresses;
  unsigned int ttl;
  void* __handle;
};

struct TDNSTXTRecords
{
  struct sockaddr_storage** addresses;
  unsigned int ttl;
  void* __handle;
};

struct TDNSMX
{
  const char* name;
  unsigned int priority;
};
  
struct TDNSMXs
{
  struct TDNSMX** mxs;
  unsigned int ttl;
  void *__handle;
};

struct TDNSContext* TDNSMakeContext(const char** servers, int server_count) {
    // Allocate memory for context
    struct TDNSContext* context = (struct TDNSContext*)malloc(sizeof(struct TDNSContext));
    if (!context) return NULL;

    context->timeoutMsec = 5000;
    context->servers = (char**)malloc(server_count * sizeof(char*));
    if (!context->servers) {
        free(context);
        return NULL;
    }

    // Copy server addresses into context
    for (int i = 0; i < server_count; i++) {
        context->servers[i] = strdup(servers[i]);
        if (!context->servers[i]) {
            // Cleanup and return NULL in case of allocation failure
            for (int j = 0; j < i; j++) free(context->servers[j]);
            free(context->servers);
            free(context);
            return NULL;
        }
    }
    
    return context;
}

// Frees the TDNS context
void freeTDNSContext(struct TDNSContext* context)
{
    if (context) {
        // Free each server string in the servers array
        for (int i = 0; context->servers[i] != NULL; i++) {
            free(context->servers[i]);  // Free each individual server string
        }
        
        // Free the servers array itself
        free(context->servers);
        
        // Finally, free the context structure itself
        free(context);
    }
}

// Looks up IP addresses (A records)
int TDNSLookupIPs(struct TDNSContext* context, const char* name, int timeoutMsec, int lookupIPv4, struct TDNSIPAddresses** ret)
{
    //context --> used for future DNS caching
    //name --> domain name-ul
    //timeoutMsec --> timpul pentru lookup, nu este folosit inca
    //ret --> un pointer care stocheaza rezultatul acestei functii
    struct hostent* host; // folosita pentru functia gethostbyname()
    struct in_addr addr; // structura care mentine adrese ipv4
    
    *ret = (struct TDNSIPAddresses*)malloc(sizeof(struct TDNSIPAddresses));
    if (!(*ret)) {
        return 1; // Memory allocation failed
    }

    // Lookup the domain name using gethostbyname
    //gethostbyname steps:
    // -se uita in local cache
    // -se uita in /etc/hosts
    // -se uita in /etc/resolv.conf
    // -in /etc/resolv.conf, trimite querry-uri catre serverul de acolo(pot fi mai multe)
    // adresa de acolo(127.0.0.53) este adresa de loopback, pointeaza catre un DNSS resolver local 
    host = gethostbyname(name);
    if (!host) {
        free(*ret);
        return 1; // DNS lookup failed
    }

    // If we're looking for IPv4 addresses
    if (lookupIPv4 && host->h_addrtype == AF_INET) {
        (*ret)->addresses = (struct sockaddr_storage**)malloc(sizeof(struct sockaddr_storage*));
        (*ret)->ttl = 60; // Set TTL (example)

        // Convert the first address to sockaddr_storage and store it
        struct sockaddr_in* sin = (struct sockaddr_in*)malloc(sizeof(struct sockaddr_in));
        sin->sin_family = AF_INET;
        memcpy(&sin->sin_addr, host->h_addr_list[0], sizeof(struct in_addr));
        (*ret)->addresses[0] = (struct sockaddr_storage*)sin;
    }

    return 0; // Success
}

// Funcția pentru a prelua adresa IP din fișierul ZONE
int TDNSLookupIPsFromZoneFile(const char* name, struct TDNSIPAddresses** ret) {
    printf("ceva1\n");
    FILE* file = fopen(zoneFilePath, "r");
    if (!file) {
        printf("ceva1\n");
        return 1; // Eroare la deschiderea fișierului
    }

    *ret = (struct TDNSIPAddresses*)malloc(sizeof(struct TDNSIPAddresses));
    if (!(*ret)) {
        fclose(file);
        printf("ceva2\n");
        return 1; // Eșec la alocarea memoriei
    }

    char line[256];
    int found = 0;

    // Parcurgem fișierul linie cu linie
    while (fgets(line, sizeof(line), file)) {
        char domain[128];
        char type[4];
        char ipStr[INET_ADDRSTRLEN];

        // Citim domeniul, tipul de înregistrare și adresa IP
        if (sscanf(line, "%127s IN %3s %15s", domain, type, ipStr) == 3) {
            // Verificăm dacă linia conține o înregistrare de tip "A" și domeniul se potrivește
            //printf("%s %s %s\n", domain, type, ipStr);  --> verificare ca luam datele corect din zone
            if (strcmp(type, "A") == 0 && strcmp(domain, name) == 0) {
                struct sockaddr_in* sin = (struct sockaddr_in*)malloc(sizeof(struct sockaddr_in));
                sin->sin_family = AF_INET;

                if (inet_pton(AF_INET, ipStr, &sin->sin_addr) == 1) {
                    // Alocăm și stocăm adresa găsită
                    (*ret)->addresses = (struct sockaddr_storage**)malloc(sizeof(struct sockaddr_storage*));
                    (*ret)->addresses[0] = (struct sockaddr_storage*)sin;
                    (*ret)->ttl = 60; // Exemplar TTL
                    found = 1;
                    break;
                } else {
                    free(sin); // Dacă adresa IP nu este validă
                }
            }
        }
    }

    fclose(file);

    if (!found) {
        free(*ret);
        *ret = NULL;
        return 1; // Domeniul nu a fost găsit în fișierul ZONE
    }

    return 0; // Succes
}


// Frees the IP addresses data structure
void freeTDNSIPAddresses(struct TDNSIPAddresses* ips)
{
    if (ips) {
        for (int i = 0; ips->addresses[i]; ++i) {
            free(ips->addresses[i]);
        }
        free(ips->addresses);
        free(ips);
    }
}

// Looks up MX records
int TDNSLookupMXs(struct TDNSContext* context, const char* name, int timeoutMsec, struct TDNSMXs** ret)
{
    struct hostent *mx_host;
    *ret = (struct TDNSMXs*)malloc(sizeof(struct TDNSMXs));

    // For simplicity, assuming the MX lookup only resolves to a single MX record (e.g., for a real application, you would query a DNS server)
    mx_host = gethostbyname(name);
    if (!mx_host) {
        return 1; // DNS lookup failed
    }

    (*ret)->mxs = (struct TDNSMX**)malloc(sizeof(struct TDNSMX*) * 2); // Example size, change as needed
    (*ret)->ttl = 60; // Set TTL for example

    (*ret)->mxs[0] = (struct TDNSMX*)malloc(sizeof(struct TDNSMX));
    (*ret)->mxs[0]->name = mx_host->h_name;
    (*ret)->mxs[0]->priority = 10; // Example priority

    return 0; // Success
}

// Frees the MX records data structure
void freeTDNSMXs(struct TDNSMXs* mxs)
{
    if (mxs) {
        for (int i = 0; mxs->mxs[i]; ++i) {
            free(mxs->mxs[i]);
        }
        free(mxs->mxs);
        free(mxs);
    }
}
  
#ifdef __cplusplus
}
#endif

  
#endif