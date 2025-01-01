#include <stdio.h>
#include <stdlib.h>

#include "dns_packet.h"
#include "dns_server.h"

// This executable is for testing whether the server works (receives and parses packets) through "dig"
// example command: dig @127.0.0.1 -p 53 google.com

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