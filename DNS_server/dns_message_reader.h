#ifndef DNS_MESSAGE_READER_H
#define DNS_MESSAGE_READER_H

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define MAX_PAYLOAD_SIZE 512

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

#endif
