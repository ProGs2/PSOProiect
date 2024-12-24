#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8081
#define BUFFER_SIZE 2048

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <domain_name>\n", argv[0]);
        return EXIT_FAILURE;
    }

    if (strlen(argv[1]) == 0 || strlen(argv[1]) > 253) {
        fprintf(stderr, "Invalid domain name: Must be non-empty and less than 254 characters.\n");
        return EXIT_FAILURE;
    }

    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};

    // Create socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error");
        return EXIT_FAILURE;
    }

    // Server address setup
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        perror("Invalid address");
        close(sock);
        return EXIT_FAILURE;
    }

    // Connect to the server
    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection failed");
        close(sock);
        return EXIT_FAILURE;
    }

    printf("Connected to server at 127.0.0.1:%d\n", PORT);

    // Send domain query
    if (send(sock, argv[1], strlen(argv[1]), 0) <= 0) {
        perror("Send failed");
        close(sock);
        return EXIT_FAILURE;
    }
    printf("Query sent: %s\n", argv[1]);

    // Receive response
    int valread = read(sock, buffer, BUFFER_SIZE);
    if (valread > 0) {
        buffer[valread] = '\0';
        printf("Response: %s\n", buffer);
    } else if (valread == 0) {
        printf("Connection closed by server.\n");
    } else {
        perror("Read failed");
    }

    // Close socket
    close(sock);
    return EXIT_SUCCESS;
}
