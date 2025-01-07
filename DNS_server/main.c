#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dns_packet.h"
#include "dns_server.h"

// This executable is for testing whether the server works (receives and parses packets) through "dig"
// example command: dig @127.0.0.1 -p 53 google.com

/*
// This was the test code for the non-forwarding section

int main(void) {
    uint16_t port = 53;
    
    printf("Starting DNS server on port %d...\n", port);
    
    if (dns_init_socket(port) < 0) {
        fprintf(stderr, "Failed to initialize DNS server\n");
        return 1;
    }

    printf("Socket initialized successfully\n");
    
    int result = dns_start_listening(example_dns_callback, NULL);
    
    dns_cleanup_socket();
    
    return result == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
*/

// This is the test code for the forwarding section
void dns_request_handler(struct dns_packet* packet, 
                        struct sockaddr_in* client_addr, 
                        void* user_data)
{
    // save client address for response
    struct sockaddr_in* saved_client = malloc(sizeof(struct sockaddr_in));
    if (!saved_client) {
        perror("Failed to allocate memory for client address");
        return;
    }
    memcpy(saved_client, client_addr, sizeof(struct sockaddr_in));
    
    // forward to Google DNS (8.8.8.8)
    int ret = dns_forward_query(packet, "8.8.8.8", 53, example_dns_forward_callback, saved_client);
    if (ret < 0) {
        printf("Failed to forward query\n");
        free(saved_client);
    }
}

int main(void) {
    if (dns_init_socket(53) < 0) {
        return 1;
    }

    // start listening with request_handler
    dns_start_listening(dns_request_handler, NULL);

    dns_cleanup_socket();
    return EXIT_SUCCESS;
}