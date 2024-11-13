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
    //sockfd -->  file descriptor-ul soketului UDP
    //client_addr --> stocheaza info despre adresa clientului(ipv4)
    //tdns --> DNS context
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

    // Perform the DNS lookup for the domain (IPv4 only)
    struct TDNSIPAddresses* ip_addresses = NULL;
    if (TDNSLookupIPs(tdns, domain, 1000, 1, &ip_addresses) != 0 || !ip_addresses) {
        fprintf(stderr, "DNS lookup failed for %s\n", domain);
        return;
    }

    // Get the first IP address from the lookup result
    struct sockaddr_in* addr_in = (struct sockaddr_in*)ip_addresses->addresses[0];
    if (!addr_in) {
        fprintf(stderr, "No IP address found for %s\n", domain);
        freeTDNSIPAddresses(ip_addresses);
        return;
    }

    // Convert the IP address to a human-readable format and print it
    char ip_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &addr_in->sin_addr, ip_str, sizeof(ip_str));
    printf("Resolved IP address for %s: %s\n", domain, ip_str);

    // Copy transaction ID from the query to the response
    char response[BUFFER_SIZE] = {0};  // Initialize the buffer to zero to avoid garbage data
    response[0] = buffer[0];
    response[1] = buffer[1];

    // Set response flags
    response[2] = 0x81; // QR=1 (response), Opcode=0 (standard query), AA=1, TC=0, RD=1
    response[3] = 0x80; // RA=1, Z=0, RCode=0 (NOERROR)

    // Set counts for Question and Answer sections
    response[4] = 0x00; response[5] = 0x01;  // QDCOUNT = 1 (Question count)
    response[6] = 0x00; response[7] = 0x01;  // ANCOUNT = 1 (Answer count)
    response[8] = 0x00; response[9] = 0x00;  // NSCOUNT = 0 (Authority count)
    response[10] = 0x00; response[11] = 0x00; // ARCOUNT = 0 (Additional count)

    // Copy question section to response (12 bytes for header, rest is question)
    memcpy(response + 12, buffer + 12, received - 12);
    int response_size = received;

    // Add a minimal answer section with an A record
    response[response_size++] = 0xc0; // Name pointer to the start of the question
    response[response_size++] = 0x0c;
    response[response_size++] = 0x00; // Type (A record)
    response[response_size++] = 0x01;
    response[response_size++] = 0x00; // Class (IN)
    response[response_size++] = 0x01;
    response[response_size++] = 0x00; response[response_size++] = 0x00;
    response[response_size++] = 0x00; response[response_size++] = 0x3c; // TTL = 60 seconds
    response[response_size++] = 0x00; response[response_size++] = 0x04;  // Data length for IPv4

    // Insert the IP address returned by TDNSLookupIPs
    memcpy(&response[response_size], &addr_in->sin_addr, sizeof(addr_in->sin_addr));
    response_size += sizeof(addr_in->sin_addr);

    // Send only the exact number of bytes in the response
    if (sendto(sockfd, response, response_size, 0, (struct sockaddr*)client_addr, addr_len) < 0) {
        perror("Failed to send DNS response");
    }
    
    // Free the IP addresses data structure after using it
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
