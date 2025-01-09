#ifndef __DNS_SERVER_H__
#define __DNS_SERVER_H__

#include <netinet/in.h>
#include "dns_packet.h"

/* ipv4 address max length */
#define INET_ADDRSTR_LEN 16

/* callback function type for packet processing */
// a callback function is any function that receives the packet from the network and processes it.
// since the program already parses the packet before this function is called, all that's left is to 
// call a function that would, for example, take the query data and search it in the dns tree to create a response.

typedef void (*dns_callback_fn) (struct dns_packet *packet, 
                                struct sockaddr_in *sender, 
                                void *user_data);

// this callback was made in order to properly integrate the code written so far with the rest of the project
typedef void (*dns_response_callback_fn) (struct dns_packet* response, 
                                         void* user_data);

/* * * * * * * */

/* server functions */
/* server socket init */
int dns_init_socket(uint16_t port);

/* server cleanup */
void dns_cleanup_socket(void);

/* send dns packet to X address */
int dns_send_packet(const struct dns_packet *pkt, 
                    const char *server_ip, 
                    uint16_t port);

/* send dns answer to same socket query was recv'd from */
int dns_send_answer(const struct dns_packet* answer_pkt, 
                    const struct sockaddr_in* client_addr);

/* start listening */
int dns_start_listening(dns_callback_fn callback, void *user_data);

/* stop listening */
void dns_stop_listening(void);

/* test callback function */
void example_dns_callback(struct dns_packet *packet, 
                         struct sockaddr_in *sender, 
                         void *user_data);

/* forwarding function */
int dns_forward_query(const struct dns_packet* query_pkt, 
                     const char* forward_ip, 
                     uint16_t forward_port,
                     dns_callback_fn callback,
                     void* user_data);

/* waiting for response after forwarding */
int dns_wait_response(uint16_t query_id, 
                     int timeout_sec,
                     void* callback,
                     void* user_data,
                     int callback_type);

/* example callback for forwarding */
void example_dns_forward_callback(struct dns_packet* packet, 
                                  struct sockaddr_in* client_addr, 
                                  void* user_data);

/* made for integration with rest of project */
// the function takes a string (domain_name) and a dns server ip (dns_server), generates a dns query packet,
// and performs a dns query, receiving the response and processing it through the callback function. 
int dns_query_domain(const char* domain_name, 
                    const char* dns_server,
                    uint16_t dns_port,
                    dns_response_callback_fn callback,
                    void* user_data);

void handle_dns_response(struct dns_packet* response, void* user_data);

#endif