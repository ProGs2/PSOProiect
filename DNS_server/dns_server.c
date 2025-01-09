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
#include <time.h>

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

// function to encode domain name into DNS wire format (fixing FormErr)
static int dns_encode_name(uint8_t* buffer, const char* domain) {
    uint8_t* label_length_ptr = buffer++;  // advance buffer after storing length ptr
    int total_length = 1;                  // start at 1 for first length byte
    int label_length = 0;
    
    while (*domain) {
        if (*domain == '.') {
            *label_length_ptr = label_length;     // write the length of current label
            label_length_ptr = buffer++;          // set up for next label, increment buffer
            label_length = 0;                     // reset for next label
            total_length++;                       // count the length byte
        } else {
            *buffer++ = *domain;                  // copy character
            label_length++;
            total_length++;
        }
        domain++;
    }
    
    // handle last label
    *label_length_ptr = label_length;
    *buffer = 0;  // add terminating zero
    
    return total_length + 1;  // include terminating zero in total length
}

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

    // serialize packet
    uint8_t buffer[512];  // standard dns udp size
    size_t offset = 0;

    // convert header to network byte order
    struct dns_header header = pkt->header;
    header.id = htons(header.id);
    header.qdcount = htons(header.qdcount);
    header.ancount = htons(header.ancount);
    header.nscount = htons(header.nscount);
    header.arcount = htons(header.arcount);

    // copy header
    memcpy(buffer + offset, &header, sizeof(struct dns_header));
    offset += sizeof(struct dns_header);

    // encode qname in dns wire format
    int name_length = dns_encode_name(buffer + offset, pkt->question.qname);
    if (name_length < 0) {
        fprintf(stderr, "Failed to encode domain name\n");
        return -1;
    }
    offset += name_length;

    // add QTYPE and QCLASS in network byte order
    uint16_t qtype = htons(pkt->question.qtype);
    uint16_t qclass = htons(pkt->question.qclass);
    
    memcpy(buffer + offset, &qtype, sizeof(uint16_t));
    offset += sizeof(uint16_t);
    memcpy(buffer + offset, &qclass, sizeof(uint16_t));
    offset += sizeof(uint16_t);

    printf("Sending DNS query to %s:%d (packet size: %zu bytes)\n", 
           server_ip, port, offset);

    ssize_t sent = sendto(g_sockfd, buffer, offset, 0,
                         (struct sockaddr*)&server_addr, sizeof(server_addr));
    
    if (sent < 0) {
        perror("Failed to send DNS query");
    } else {
        printf("Successfully sent %zd bytes\n", sent);
    }
    
    return sent;
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

// forwarding function
int dns_forward_query(const struct dns_packet* query_pkt, 
                     const char* forward_ip, 
                     uint16_t forward_port,
                     dns_callback_fn callback,
                     void* user_data)
{
    if (g_sockfd == -1) {
        fprintf(stderr, "Socket not initialized\n");
        return -1;
    }

    // forward query to other dns server
    int result = dns_send_packet(query_pkt, forward_ip, forward_port);
    if (result < 0) {
        fprintf(stderr, "Failed to send forwarded query\n");
        return -1;
    }

    // wait for response
    return dns_wait_response(query_pkt->header.id, 5, callback, user_data, 0);
}

// wait for a dns response (and do something with the response through callback)
int dns_wait_response(uint16_t query_id, 
                     int timeout_sec,
                     void* callback,
                     void* user_data,
                     int callback_type)
{
    struct timeval tv;
    fd_set readfds;
    uint8_t buffer[512];
    struct sockaddr_in sender_addr;
    socklen_t sender_len = sizeof(sender_addr);
    struct dns_packet response_pkt;
    char sender_ip[INET_ADDRSTR_LEN];
    time_t start_time = time(NULL);

    while ((time(NULL) - start_time) < timeout_sec) {
        // set timeout for select() to 1 second to allow loop continuation
        tv.tv_sec = 1;
        tv.tv_usec = 0;

        // clear fd set and add socket
        FD_ZERO(&readfds);
        FD_SET(g_sockfd, &readfds);

        // wait for response
        int ready = select(g_sockfd + 1, &readfds, NULL, NULL, &tv);
        
        if (ready < 0) {
            if (errno == EINTR) continue;
            perror("select() failed");
            return -1;
        }
        
        if (ready == 0) {
            continue;  // no data yet, but not timed out; continue to wait
        }

        // recv response
        memset(buffer, 0, sizeof(buffer));
        ssize_t received = recvfrom(g_sockfd, buffer, sizeof(buffer), 0,
                                  (struct sockaddr*)&sender_addr, &sender_len);
        
        if (received < 0) {
            if (errno == EINTR) continue;
            perror("recvfrom() failed");
            return -1;
        }

        // parse response
        if (dns_request_parse(&response_pkt, buffer) == 0) {
            // sender address to string for debugging
            inet_ntop(AF_INET, &sender_addr.sin_addr, sender_ip, INET_ADDRSTR_LEN);
            printf("Received response from %s:%d\n", 
                   sender_ip, ntohs(sender_addr.sin_port));

            // check if this is the response we wanted
            if (response_pkt.header.id == query_id && response_pkt.header.qr == QR_RESPONSE) {
                // call callback with response
                if (callback) {
                    if (callback_type == 0) {  // dns_callback_fn
                    dns_callback_fn server_cb = (dns_callback_fn)callback;
                    server_cb(&response_pkt, &sender_addr, user_data);
                    } else {  // dns_response_callback_fn
                        dns_response_callback_fn resp_cb = (dns_response_callback_fn)callback;
                        resp_cb(&response_pkt, user_data);
                    }
                }
                // // // // // //

                // clean up
                if (response_pkt.question.qname) {
                    free(response_pkt.question.qname);
                }
                if (response_pkt.header.ancount > 0 && response_pkt.answer.name) {
                    free(response_pkt.answer.name);
                    if (response_pkt.answer.rdata) {
                        free(response_pkt.answer.rdata);
                    }
                }
                return 0;
            }

            // if not response we wanted, clean up
            if (response_pkt.question.qname) {
                free(response_pkt.question.qname);
            }
            if (response_pkt.header.ancount > 0 && response_pkt.answer.name) {
                free(response_pkt.answer.name);
                if (response_pkt.answer.rdata) {
                    free(response_pkt.answer.rdata);
                }
            }
        }
    }

    return -1;
}

// example of callback function for forwarding
void example_dns_forward_callback(struct dns_packet* packet, 
                                  struct sockaddr_in* client_addr, 
                                  void* user_data)
{
    printf("Processing forwarded DNS response:\n");
    dns_print_packet(packet);
    
    // forward response back to original client
    if (user_data) {  // user_data client address
        struct sockaddr_in* original_client = (struct sockaddr_in*)user_data;
        printf("Forwarding response back to original client %s:%d\n",
               inet_ntoa(original_client->sin_addr),
               ntohs(original_client->sin_port));
        
        int ret = dns_send_answer(packet, original_client);
        if (ret < 0) {
            perror("Failed to send response to original client");
        }
        
        // free saved client address
        free(user_data);
    }
}

int dns_query_domain(const char* domain_name, 
                    const char* dns_server,
                    uint16_t dns_port,
                    dns_response_callback_fn callback,
                    void* user_data)
{
    // create UDP socket specifically for this query
    // this is necessary since the server in mainDNS uses TCP
    int query_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (query_socket < 0) {
        perror("Failed to create query socket");
        return -1;
    }

    // bind to any available local port
    struct sockaddr_in local_addr;
    memset(&local_addr, 0, sizeof(local_addr));
    local_addr.sin_family = AF_INET;
    local_addr.sin_addr.s_addr = INADDR_ANY;
    local_addr.sin_port = htons(0);  // os will choose an available port

    if (bind(query_socket, (struct sockaddr*)&local_addr, sizeof(local_addr)) < 0) {
        perror("Failed to bind query socket");
        close(query_socket);
        return -1;
    }

    // get port number that was assigned
    socklen_t len = sizeof(local_addr);
    if (getsockname(query_socket, (struct sockaddr*)&local_addr, &len) < 0) {
        perror("Failed to get socket name");
        close(query_socket);
        return -1;
    }

    printf("Query socket bound to port %d\n", ntohs(local_addr.sin_port));

    // temp. store this socket in my global socket desc. so rewriting functions isnt necessary
    g_sockfd = query_socket;

    // create dns packet for query
    struct dns_packet* query_pkt = dns_create_query_packet(domain_name);
    if (!query_pkt) {
        close(query_socket);
        g_sockfd = -1;
        return -1;
    }
    
    printf("Sending DNS query for %s to %s:%d\n", 
           domain_name, dns_server, dns_port);
    
    // send query
    int send_result = dns_send_packet(query_pkt, dns_server, dns_port);

    // store id and clean up query packet
    uint16_t query_id = query_pkt->header.id;
    dns_free_packet(query_pkt);

    // if sending failed...
    if (send_result < 0) {
        fprintf(stderr, "Failed to send DNS query\n");
        close(query_socket);
        g_sockfd = -1;
        return -1;
    }
    
    // wait for response with 5 second timeout
    // this is where the dns response goes, and where the callback function acts
    int result = dns_wait_response(query_id, 5, (dns_callback_fn)callback, user_data, 1);

    // clean up
    close(query_socket);
    g_sockfd = -1;

    return result;
}

// callback function for handling the dns response
void handle_dns_response(struct dns_packet* response, void* user_data) {
    char* ip_storage = (char*)user_data;
    
    printf("DNS Response received:\n");
    printf("Answer count: %d\n", response->header.ancount);
    if (response->header.ancount > 0) {
        printf("Answer type: %d (A record = %d)\n", response->answer.type, DNS_TYPE_A);
    }
    
    if (response->header.ancount > 0 && response->answer.type == DNS_TYPE_A) {
        struct in_addr addr;
        memcpy(&addr.s_addr, response->answer.rdata, 4);
        
        // try storing in temp buffer
        char temp_ip[INET_ADDRSTR_LEN];
        inet_ntop(AF_INET, &addr.s_addr, temp_ip, INET_ADDRSTR_LEN);
        printf("Converted IP: %s\n", temp_ip);
        
        // if ok, copy to provided storage
        strncpy(ip_storage, temp_ip, INET_ADDRSTR_LEN - 1);
        ip_storage[INET_ADDRSTR_LEN - 1] = '\0';  // ensure null termination
        
        printf("Stored IP in buffer: %s\n", ip_storage);
    } else {
        printf("No valid A record found in response\n");
    }
}