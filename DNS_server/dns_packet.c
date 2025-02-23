#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>

#include "dns_packet.h"

/* utility Functions */

// measure length of dns name field; supports normal and compressed formats
size_t util_measure_name(const void* data, uint16_t offset) {
    size_t length = 0;
    const uint8_t* data_ptr = (const uint8_t*)data + offset;
    
    while (*data_ptr != 0) {
        // Check for DNS name compression (first two bits set to 1)
        if ((*data_ptr & 0xC0) == 0xC0) {
            return length + 2; // compressed names use 2 bytes
        }
        
        uint8_t label_length = *data_ptr;
        length += label_length + 1; // add label length and length byte itself
        data_ptr += label_length + 1;
    }
    
    return length + 1; // include the terminating zero byte
}

// reads a dns name from the packet, handling compression
int dns_read_name(char* dest, const void* data, uint16_t offset, size_t max_len) {
    const uint8_t* data_ptr = (const uint8_t*)data + offset;
    size_t dest_offset = 0;
    uint8_t jumped = 0;
    uint16_t jump_count = 0;
    uint16_t total_offset = offset;
    
    while (*data_ptr != 0) {
        // Handle compression
        if ((*data_ptr & 0xC0) == 0xC0) {
            if (!jumped) {
                total_offset += 2;
                jumped = 1;
            }
            
            uint16_t jump_offset = ((*data_ptr & 0x3F) << 8) | *(data_ptr + 1);
            data_ptr = (const uint8_t*)data + jump_offset;
            continue;
        }
        
        // normal label
        uint8_t label_length = *data_ptr++;
        if (!jumped) total_offset++;
        
        // Prevent buffer overflow
        if (dest_offset + label_length + 1 >= max_len) {
            return -1;
        }
        
        // Copy the label and add a dot
        memcpy(dest + dest_offset, data_ptr, label_length);
        dest_offset += label_length;
        dest[dest_offset++] = '.';
        
        data_ptr += label_length;
        if (!jumped) total_offset += label_length;
    }
    
    if (!jumped) total_offset++;
    
    if (dest_offset > 0) {
        dest[dest_offset - 1] = '\0'; // replace last dot with null terminator
    } else {
        dest[0] = '\0';
    }
    
    return jumped ? total_offset : total_offset;
}

// creates a query packet to use in forwarding
struct dns_packet* dns_create_query_packet(const void* in_qname) {
    struct dns_packet* packet = malloc(sizeof(struct dns_packet));
    if (!packet) {
        fprintf(stderr, "Failed to allocate memory for packet\n");
        return NULL;
    }
    
    // init all fields to 0
    memset(packet, 0, sizeof(struct dns_packet));
    
    // header setup
    packet->header.id = (uint16_t)rand();  // random query id
    packet->header.qr = QR_QUERY;
    packet->header.opcode = OPCODE_QUERY;
    packet->header.rd = 1;
    packet->header.qdcount = 1;
    
    // question setup
    packet->question.qname = strdup(in_qname);
    if (!packet->question.qname) {
        fprintf(stderr, "Failed to allocate memory for domain name\n");
        free(packet);
        return NULL;
    }
    packet->question.qtype = DNS_TYPE_A;
    packet->question.qclass = DNS_CLASS_IN;
    
    return packet;
}

// cleanup function for query packet
void dns_free_packet(struct dns_packet* packet) {
    if (packet) {
        if (packet->question.qname) {
            free(packet->question.qname);
        }
        if (packet->header.ancount > 0) {
            if (packet->answer.name) {
                free(packet->answer.name);
            }
            if (packet->answer.rdata) {
                free(packet->answer.rdata);
            }
        }
        free(packet);
    }
}

/* parsing Functions */

int dns_header_parse(struct dns_header* header, const void* data) {
    // copy raw header data
    memcpy(header, data, sizeof(struct dns_header));
    
    // convert multi-byte integers from network byte order
    header->id = ntohs(header->id);
    header->qdcount = ntohs(header->qdcount);
    header->ancount = ntohs(header->ancount);
    header->nscount = ntohs(header->nscount);
    header->arcount = ntohs(header->arcount);
    
    return 0;
}

int dns_question_parse(struct dns_question* question, const void* data, size_t* offset) {
    // read name
    char name_buffer[256];
    int name_end = dns_read_name(name_buffer, data, *offset, sizeof(name_buffer));
    if (name_end < 0) {
        return -1;
    }
    
    *offset = name_end;
    
    // allocate and copy name
    question->qname = strdup(name_buffer);
    if (!question->qname) {
        return -1;
    }
    
    // read type and class
    const uint8_t* cur_ptr = (const uint8_t*)data + *offset;
    memcpy(&question->qtype, cur_ptr, sizeof(uint16_t));
    memcpy(&question->qclass, cur_ptr + sizeof(uint16_t), sizeof(uint16_t));
    
    question->qtype = ntohs(question->qtype);
    question->qclass = ntohs(question->qclass);
    
    *offset += 4; // advance past TYPE and CLASS
    
    return 0;
}

int dns_answer_parse(struct dns_answer* answer, const void* data) {
    // calculate offset to answer section
    const uint8_t* cur_ptr = (const uint8_t*)data + sizeof(struct dns_header);
    
    // skip question section
    char name_buffer[256];
    int offset = dns_read_name(name_buffer, data, sizeof(struct dns_header), sizeof(name_buffer));
    if (offset < 0) {
        return -1;
    }
    offset += 4; // skip QTYPE and QCLASS
    
    // read answer name
    int name_end = dns_read_name(name_buffer, data, offset, sizeof(name_buffer));
    if (name_end < 0) {
        return -1;
    }
    
    // allocate and copy name
    answer->name = strdup(name_buffer);
    if (!answer->name) {
        return -1;
    }
    
    // read fixed-length fields
    cur_ptr = (const uint8_t*)data + name_end;
    memcpy(&answer->type, cur_ptr, sizeof(uint16_t));
    memcpy(&answer->class, cur_ptr + sizeof(uint16_t), sizeof(uint16_t));
    memcpy(&answer->ttl, cur_ptr + 2 * sizeof(uint16_t), sizeof(uint32_t));
    memcpy(&answer->rdlength, cur_ptr + 2 * sizeof(uint16_t) + sizeof(uint32_t), sizeof(uint16_t));
    
    // Convert to host byte order
    answer->type = ntohs(answer->type);
    answer->class = ntohs(answer->class);
    answer->ttl = ntohl(answer->ttl);
    answer->rdlength = ntohs(answer->rdlength);
    
    // Read RDATA
    cur_ptr += 2 * sizeof(uint16_t) + sizeof(uint32_t) + sizeof(uint16_t);
    answer->rdata = malloc(answer->rdlength + 1);
    if (!answer->rdata) {
        free(answer->name);
        return -1;
    }
    
    memcpy(answer->rdata, cur_ptr, answer->rdlength);
    answer->rdata[answer->rdlength] = '\0';
    
    return 0;
}

int dns_request_parse(struct dns_packet* pkt, const void* data) {
    size_t offset = 0;
    
    // parse header
    if (dns_header_parse(&pkt->header, data) < 0) {
        return -1;
    }
    offset += sizeof(struct dns_header);
    
    // parse question
    if (dns_question_parse(&pkt->question, data, &offset) < 0) {
        return -1;
    }
    
    // parse answer if present
    if (pkt->header.ancount > 0) {
        if (dns_answer_parse(&pkt->answer, data) < 0) {
            return -1;
        }
    }
    
    return 0;
}

/* printing Functions */

void dns_print_packet(const struct dns_packet* packet) {
    printf("DNS Packet:\n");
    printf("==========\n\n");
    
    dns_print_header(&packet->header);
    printf("\n");
    
    dns_print_question(&packet->question);
    printf("\n");
    
    if (packet->header.ancount > 0) {
        dns_print_answer(&packet->answer);
        printf("\n");
    }
}

void dns_print_header(const struct dns_header* header) {
    printf("Header:\n");
    printf("-------\n");
    printf("ID: %d\n", header->id);
    printf("QR: %d (%s)\n", header->qr, header->qr ? "Response" : "Query");
    printf("Opcode: %d\n", header->opcode);
    printf("AA: %d\n", header->aa);
    printf("TC: %d\n", header->tc);
    printf("RD: %d\n", header->rd);
    printf("RA: %d\n", header->ra);
    printf("Z: %d\n", header->z);
    printf("RCODE: %d\n", header->rcode);
    printf("QDCOUNT: %d\n", header->qdcount);
    printf("ANCOUNT: %d\n", header->ancount);
    printf("NSCOUNT: %d\n", header->nscount);
    printf("ARCOUNT: %d\n", header->arcount);
}

void dns_print_question(const struct dns_question* question) {
    printf("Question:\n");
    printf("---------\n");
    printf("QNAME: %s\n", question->qname);
    printf("QTYPE: %d\n", question->qtype);
    printf("QCLASS: %d\n", question->qclass);
}

void dns_print_answer(const struct dns_answer* answer) {
    printf("Answer:\n");
    printf("-------\n");
    printf("NAME: %s\n", answer->name);
    printf("TYPE: %d\n", answer->type);
    printf("CLASS: %d\n", answer->class);
    printf("TTL: %d\n", answer->ttl);
    printf("RDLENGTH: %d\n", answer->rdlength);
    printf("RDATA: %s\n", answer->rdata);
}
