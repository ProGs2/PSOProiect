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
#include <signal.h>
#include <fcntl.h>

#include "dns_packet.h"
#include "dns_server.h"

// global socket descriptor
static int g_sockfd = -1;

// flag for listen loop control
static volatile int g_keep_running = 1;

// graceful shutdown signal handler
void handle_signal(int signum) {
    g_keep_running = 0;
}

// persistent socket init
int dns_init_socket(uint16_t port) {
    if (g_sockfd != -1) {
        // socket already initialized!
        return 0;
    }

    // create udp socket
    g_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (g_sockfd < 0) {
        perror("Failed to create socket");
        return -1;
    }

    // set socket options for reusing address
    int reuse = 1;
    if (setsockopt(g_sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        perror("Failed to set SO_REUSEADDR");
        close(g_sockfd);
        g_sockfd = -1;
        return -1;
    }

    // bind socket to specified port
    struct sockaddr_in bind_addr;
    memset(&bind_addr, 0, sizeof(bind_addr));
    bind_addr.sin_family = AF_INET;
    bind_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    bind_addr.sin_port = htons(port);

    if (bind(g_sockfd, (struct sockaddr*)&bind_addr, sizeof(bind_addr)) < 0) {
        perror("Failed to bind socket");
        close(g_sockfd);
        g_sockfd = -1;
        return -1;
    }

    // setup signal handler for graceful shutdown
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    return 0;
}

// clean up socket resources
void dns_cleanup_socket(void) {
    if (g_sockfd != -1) {
        close(g_sockfd);
        g_sockfd = -1;
    }
}

// send query packet using persistent socket
int dns_send_packet(const struct dns_packet* pkt, const char* server_ip, uint16_t port) {
    if (g_sockfd == -1) {
        fprintf(stderr, "Socket not initialized\n");
        return -1;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        return -1;
    }

    // serialize packet (same as before)
    uint8_t buffer[512];
    size_t offset = 0;

    // header
    memcpy(buffer + offset, &pkt->header, sizeof(struct dns_header));
    offset += sizeof(struct dns_header);

    // question
    size_t qname_len = strlen(pkt->question.qname) + 1;
    memcpy(buffer + offset, pkt->question.qname, qname_len);
    offset += qname_len;
    memcpy(buffer + offset, &pkt->question.qtype, sizeof(uint16_t));
    offset += sizeof(uint16_t);
    memcpy(buffer + offset, &pkt->question.qclass, sizeof(uint16_t));
    offset += sizeof(uint16_t);

    return sendto(g_sockfd, buffer, offset, 0,
                 (struct sockaddr*)&server_addr, sizeof(server_addr));
}

// send answer packet to a client
int dns_send_answer(const struct dns_packet* answer_pkt, const struct sockaddr_in* client_addr) {
    if (g_sockfd == -1) {
        fprintf(stderr, "Socket not initialized\n");
        return -1;
    }

    // buffer for serialized packet
    uint8_t buffer[512];  // standard dns udp size
    size_t offset = 0;

    // serialize header
    struct dns_header header = answer_pkt->header;
    // ensure packet is marked as a response
    header.qr = QR_RESPONSE;
    
    // convert header fields to network byte order
    header.id = htons(header.id);
    header.qdcount = htons(header.qdcount);
    header.ancount = htons(header.ancount);
    header.nscount = htons(header.nscount);
    header.arcount = htons(header.arcount);

    memcpy(buffer + offset, &header, sizeof(struct dns_header));
    offset += sizeof(struct dns_header);

    // serialize question section (should match the query)
    size_t qname_len = strlen(answer_pkt->question.qname) + 1;
    memcpy(buffer + offset, answer_pkt->question.qname, qname_len);
    offset += qname_len;

    uint16_t qtype = htons(answer_pkt->question.qtype);
    uint16_t qclass = htons(answer_pkt->question.qclass);
    
    memcpy(buffer + offset, &qtype, sizeof(uint16_t));
    offset += sizeof(uint16_t);
    memcpy(buffer + offset, &qclass, sizeof(uint16_t));
    offset += sizeof(uint16_t);

    // serialize answer section (if present)
    if (answer_pkt->header.ancount > 0) {
        // answer name
        size_t name_len = strlen(answer_pkt->answer.name) + 1;
        memcpy(buffer + offset, answer_pkt->answer.name, name_len);
        offset += name_len;

        // answer fixed fields
        uint16_t type = htons(answer_pkt->answer.type);
        uint16_t class = htons(answer_pkt->answer.class);
        uint32_t ttl = htonl(answer_pkt->answer.ttl);
        uint16_t rdlength = htons(answer_pkt->answer.rdlength);

        memcpy(buffer + offset, &type, sizeof(uint16_t));
        offset += sizeof(uint16_t);
        memcpy(buffer + offset, &class, sizeof(uint16_t));
        offset += sizeof(uint16_t);
        memcpy(buffer + offset, &ttl, sizeof(uint32_t));
        offset += sizeof(uint32_t);
        memcpy(buffer + offset, &rdlength, sizeof(uint16_t));
        offset += sizeof(uint16_t);

        // answer data
        memcpy(buffer + offset, answer_pkt->answer.rdata, answer_pkt->answer.rdlength);
        offset += answer_pkt->answer.rdlength;
    }

    // send answer packet to client
    ssize_t sent = sendto(g_sockfd, buffer, offset, 0,
                         (struct sockaddr*)client_addr, sizeof(struct sockaddr_in));
    
    if (sent < 0) {
        perror("Failed to send DNS answer");
        return -1;
    }

    return 0;
}

// start listening for packets
int dns_start_listening(dns_callback_fn callback, void* user_data) {
    if (g_sockfd == -1) {
        fprintf(stderr, "Socket not initialized\n");
        return -1;
    }

    // get the bound port number for debugging
    struct sockaddr_in bound_addr;
    socklen_t bound_len = sizeof(bound_addr);
    if (getsockname(g_sockfd, (struct sockaddr*)&bound_addr, &bound_len) == 0) {
        printf("DNS server listening on port %d...\n", ntohs(bound_addr.sin_port));
    }

    uint8_t buffer[512];  // standard DNS UDP size
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    struct dns_packet received_packet;
    char client_ip[INET_ADDRSTR_LEN];

    while (g_keep_running) {
        // clear the buffer before receiving new data
        memset(buffer, 0, sizeof(buffer));
        
        // receive incoming packet
        ssize_t received = recvfrom(g_sockfd, buffer, sizeof(buffer), 0,
                                  (struct sockaddr*)&client_addr, &client_len);

        if (received < 0) {
            if (errno == EINTR) {
                // interrupted by signal, check if we should continue running
                continue;
            }
            perror("Error receiving DNS query");
            continue;
        }

        // convert client address to string for debugging
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTR_LEN);
        printf("\nReceived %zd bytes from %s:%d\n", 
               received, client_ip, ntohs(client_addr.sin_port));

        // ensure enough data to constitute a DNS header was recv'd
        if (received < (ssize_t)sizeof(struct dns_header)) {
            fprintf(stderr, "Received packet too small for DNS header\n");
            continue;
        }

        // init packet structure
        memset(&received_packet, 0, sizeof(received_packet));

        // parse recv'd packet
        if (dns_request_parse(&received_packet, buffer) == 0) {
            printf("Successfully parsed DNS packet\n");
            
            // debug print the parsed packet
            dns_print_packet(&received_packet);

            // call user callback with parsed query and client address
            if (callback) {
                callback(&received_packet, &client_addr, user_data);
            }

            // clean up parsed packet
            if (received_packet.question.qname) {
                free(received_packet.question.qname);
            }
            if (received_packet.header.ancount > 0 && received_packet.answer.name) {
                free(received_packet.answer.name);
                if (received_packet.answer.rdata) {
                    free(received_packet.answer.rdata);
                }
            }
        } else {
            fprintf(stderr, "Failed to parse DNS packet\n");
        }
    }

    return 0;
}

// test callback function
void example_dns_callback(struct dns_packet* packet, struct sockaddr_in* sender, void* user_data) {
    char sender_ip[INET_ADDRSTR_LEN];
    inet_ntop(AF_INET, &(sender->sin_addr), sender_ip, INET_ADDRSTR_LEN);

    printf("\nReceived DNS packet from %s:%d\n", 
           sender_ip, ntohs(sender->sin_port));
    dns_print_packet(packet);
}

// stop listening for packets
void dns_stop_listening(void) {
    g_keep_running = 0;
}