#include "tdns-c.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define DNS_PORT 5355
#define BUFFER_SIZE 512

void handle_dns_query(int sockfd, struct sockaddr_in *client_addr, struct TDNSContext* tdns) {
    char buffer[BUFFER_SIZE];
    socklen_t addr_len = sizeof(*client_addr);

    // Receive DNS query
    int received = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr*)client_addr, &addr_len);
    if (received < 0) {
        perror("Failed to receive DNS query");
        return;
    }

    // Extract the domain name from the query
    char domain[256] = {0};
    int i = 12, j = 0, label_length = buffer[i++];
    while (label_length > 0 && i < received && j < sizeof(domain) - 1) {
        while (label_length-- > 0 && i < received && j < sizeof(domain) - 1) {
            domain[j++] = buffer[i++];
        }
        domain[j++] = '.';
        label_length = buffer[i++];
    }
    domain[j - 1] = '\0';  // Null-terminate the domain string

    printf("Received query for domain: %s\n", domain);

    // First try to resolve from the zone file
    struct TDNSIPAddresses* ip_addresses = NULL;
    int result = TDNSLookupIPsFromZoneFile(domain, &ip_addresses);

    // If the domain was not found in the zone file, try an external DNS lookup
    if (result != 0) {
        printf("Domain not found in zone file. Trying external DNS lookup.\n");
        if (TDNSLookupIPs(tdns, domain, 1000, 1, &ip_addresses) != 0 || !ip_addresses) {
            fprintf(stderr, "DNS lookup failed for %s\n", domain);
            return;
        }
    }

    // Process the IP address if found
    struct sockaddr_in* addr_in = (struct sockaddr_in*)ip_addresses->addresses[0];
    if (!addr_in) {
        fprintf(stderr, "No IP address found for %s\n", domain);
        freeTDNSIPAddresses(ip_addresses);
        return;
    }

    char ip_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &addr_in->sin_addr, ip_str, sizeof(ip_str));
    printf("Resolved IP address for %s: %s\n", domain, ip_str);

    // Create a DNS response with the resolved IP address
    char response[BUFFER_SIZE] = {0};
    response[0] = buffer[0];
    response[1] = buffer[1];
    response[2] = 0x81;
    response[3] = 0x80;
    response[4] = 0x00; response[5] = 0x01;
    response[6] = 0x00; response[7] = 0x01;
    response[8] = 0x00; response[9] = 0x00;
    response[10] = 0x00; response[11] = 0x00;
    memcpy(response + 12, buffer + 12, received - 12);
    int response_size = received;
    response[response_size++] = 0xc0;
    response[response_size++] = 0x0c;
    response[response_size++] = 0x00;
    response[response_size++] = 0x01;
    response[response_size++] = 0x00;
    response[response_size++] = 0x01;
    response[response_size++] = 0x00; response[response_size++] = 0x00;
    response[response_size++] = 0x00; response[response_size++] = 0x3c;
    response[response_size++] = 0x00; response[response_size++] = 0x04;
    memcpy(&response[response_size], &addr_in->sin_addr, sizeof(addr_in->sin_addr));
    response_size += sizeof(addr_in->sin_addr);

    // Send response back to the client
    if (sendto(sockfd, response, response_size, 0, (struct sockaddr*)client_addr, addr_len) < 0) {
        perror("Failed to send DNS response");
    }

    // Free memory for IP addresses
    freeTDNSIPAddresses(ip_addresses);
}

int main() {
   // Define hint servers
    const char* hint_servers[] = {"8.8.8.8", "8.8.4.4"}; //future use :)
    int nr_hint_servers = 2;

    // Initialize DNS context
    struct TDNSContext* tdns = TDNSMakeContext(hint_servers, nr_hint_servers);
    if (!tdns) {
        fprintf(stderr, "Unable to initialize TDNS context\n");
        return EXIT_FAILURE;
    }

    // Create UDP socket
    //AF_INET means that we use the ipv4
    //SOCK_DGRAM means that we use UDP
    //0 este pentru alegerea UDP
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Failed to create socket");
        freeTDNSContext(tdns);
        return EXIT_FAILURE;
    }

    struct sockaddr_in server_addr; //stores info about server's address(pt ipv4)
    memset(&server_addr, 0, sizeof(server_addr)); //initializam cu 0 ca sa ne asiguram ca nu avem alte date acolo in mem.
    server_addr.sin_family = AF_INET; // socket-ul o sa foloseasca adrese ipv4
    server_addr.sin_port = htons(DNS_PORT); // converteste portul din host byte order in network byte order
    server_addr.sin_addr.s_addr = INADDR_ANY; // server-ul asculta pe toate adresele posibile (0.0.0.0)
    // folosit daca avem mai multe interfete de retea

    if (bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Failed to bind socket");    
        close(sockfd);
        freeTDNSContext(tdns);
        return EXIT_FAILURE;
    }

    printf("DNS server is listening on port %d...\n", DNS_PORT);

    // Main server loop
    while (1) {
        struct sockaddr_in client_addr;
        handle_dns_query(sockfd, &client_addr, tdns);
    }

    // Clean up
    close(sockfd);
    freeTDNSContext(tdns);

    return 0;
}
