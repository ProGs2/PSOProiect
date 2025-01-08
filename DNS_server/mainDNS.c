#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <arpa/inet.h>
#include "trie.h"
#include "cache.h"
#include "thread.h"
#include "logger.h"
#include "dns_server.h"

// graceful shutdown stuff
// if ctrl+c is entered, the wile(1) loop at the end of the program will not repeat, thus the clean up functions
// will be reached
static volatile sig_atomic_t server_running = 1;
void handle_sigint(int sig) {
    printf("\nServer will shut down after next request.\n");
    server_running = 0;
}

typedef struct {
    struct TrieNode* root;
    struct DNSCache* cache;
    Logger* logger;
} ServerContext;

typedef struct {
    int client_socket;
    ServerContext* context;
} ClientTask;

#define PORT 8081
#define BUFFER_SIZE 1024

void printTrie(struct TrieNode* node, int level) {
    if (!node) return;

    for (int i = 0; i < level; i++) {
        printf("  ");
    }

    printf("|-- %s\n", node->label);

    for (int i = 0; i < node->nr_childrens; i++) {
        printTrie(node->childrens[i], level + 1);
    }
}
void writeTrieToDot(struct TrieNode* node, FILE* file, int parentId) {
    static int nodeId = 0; //este necesar pentru programul asta, altfel nu merge
    int currentId = nodeId++;
    fprintf(file, "    node%d [label=\"%s\"];\n", currentId, node->label);

    if (parentId != -1) {
        fprintf(file, "    node%d -> node%d;\n", parentId, currentId);
    }

    for (int i = 0; i < node->nr_childrens; i++) {
        writeTrieToDot(node->childrens[i], file, currentId);
    }
}

void visualizeTrie(struct TrieNode* root) {
    FILE* dotFile = fopen("trie.dot", "w");
    if (!dotFile) {
        perror("Failed to create .dot file for Trie visualization");
        return;
    }

    //aici avem header-ul, specificam forma nodurilor, etc
    fprintf(dotFile, "digraph Trie {\n");
    fprintf(dotFile, "    node [shape=circle];\n");

    writeTrieToDot(root, dotFile, -1);

    fprintf(dotFile, "}\n");
    fclose(dotFile);

    system("dot -Tpng trie.dot -o trie.png");

    system("xdg-open trie.png");
}

void handleClient(void* arg) {
    ClientTask* task = (ClientTask*)arg; // Cast argument to ClientTask*
    int client_socket = task->client_socket;
    ServerContext* context = task->context;
    free(task);

    logMessage(context->logger, "INFO", "Handling client socket: %d", client_socket);

    char buffer[BUFFER_SIZE] = {0};
    int valread = read(client_socket, buffer, BUFFER_SIZE);

    if (valread <= 0) {
        logMessage(context->logger, "ERROR", "Failed to read from client socket: %d", client_socket);
        close(client_socket);
        return;
    }

    buffer[valread] = '\0';
    logMessage(context->logger, "INFO", "Received query from client %d: %s", client_socket, buffer);
    // "buffer" contains query string (i.e. google.com)

    // Check for "trie" command
    if (strcmp(buffer, "trie") == 0) {
        logMessage(context->logger, "INFO", "Client requested Trie visualization.");
        visualizeTrie(context->root);
        char* response = "Trie visualization opened on the server.";
        send(client_socket, response, strlen(response), 0);
        close(client_socket);
        return;
    }

    // CacheEntry object is created ONLY if the qname is found within the tree/cache
    // If retrieveValue can't find the requested domain name, it returns NULL
    // With this in mind, I do not need to use cache_entry in the forwarding handler
    struct CacheEntry* cache_entry = retriveValue(context->root, buffer, context->cache);

    if (cache_entry) {
        logMessage(context->logger, "INFO", "Cache/Trie hit for query %s -> %s", buffer, cache_entry->record_value);
    } else {
        logMessage(context->logger, "INFO", "Domain not found for query: %s", buffer);
    }

    if (cache_entry) {
        addCacheEntry(context->cache, cache_entry);
        logMessage(context->logger, "INFO", "Added query result to cache: %s", buffer);
    } else {
        // If program enters here, it means that the requested domain name does not exist locally and must be obtained
        // through forwarding.
        // Forwarding handler will receive buffer (== domain_name)
        // [Insert functions to forward and load into cache HERE]
        char ip_address[INET_ADDRSTR_LEN] = {0};
        dns_query_domain(buffer, "1.1.1.1", 53, handle_dns_response, &ip_address);
        logMessage(context->logger, "INFO", "Finished forwarding query. Received: %s", ip_address);
        cache_entry = dns_createNewEntry(buffer, ip_address);
        addCacheEntry(context->cache, cache_entry);
        logMessage(context->logger, "INFO", "Added forwarded query result to cache: %s", buffer);
    }

    // Prepare the response
    char* response;
    if (cache_entry && cache_entry->record_value) {
        response = cache_entry->record_value;
    } else {
        // OLD!! response = "Record not found";
        // Will need to handle case where, even after forwarding, there is no response
        printf("todo\n");
    }

    // Send the response back to the client
    if (send(client_socket, response, strlen(response), 0) < 0) {
        logMessage(context->logger, "ERROR", "Failed to send response to client socket: %d", client_socket);
    } else {
        logMessage(context->logger, "INFO", "Sent response to client %d: %s", client_socket, response);
    }

    close(client_socket);
    logMessage(context->logger, "INFO", "Closed connection for client socket: %d", client_socket);
}

int main() {
    // Initialize the trie
    struct TrieNode* root = createTrieROOT();
    // Populate trie with domain data
    char** domains = getArrayOfDomainNames();
    int nr_branches = getNrBranches();
    for (int i = 0; i < nr_branches; i++) {
        struct TrieNode* branch = createBranch(domains[i]);
        root->childrens[i] = branch;
    }

    printf("Trie Structure:\n");
    printTrie(root, 0);

    // Initialize cache
    struct DNSCache* cache = initializeDNSCache();

    Logger* logger = initLogger("dns_server.log");
    if (!logger) {
        fprintf(stderr, "Failed to initialize logger. Exiting...\n");
        return EXIT_FAILURE;
    }

    // Create shared server context
    ServerContext context = { .root = root, .cache = cache, .logger = logger };

    // Initialize thread pool
    ThreadPool* pool = initThreadPool(5);
    logMessage(logger, "INFO", "DNS server initialized. Listening on port %d", PORT);

    // prepare signal handling for ctrl+c
    struct sigaction sa = {
        .sa_handler = handle_sigint,
        .sa_flags = 0
    };
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);

    // Set up the server socket
    int server_fd;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        logMessage(logger, "ERROR", "Socket creation failed");
        perror("Socket failed");
        return EXIT_FAILURE;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        logMessage(logger, "ERROR", "Bind failed");
        perror("Bind failed");
        close(server_fd);
        return EXIT_FAILURE;
    }

    if (listen(server_fd, 3) < 0) {
        logMessage(logger, "ERROR", "Listen failed");
        perror("Listen failed");
        close(server_fd);
        return EXIT_FAILURE;
    }

    logMessage(logger, "INFO", "Server is listening on port %d", PORT);
    signal(SIGINT, handle_sigint);

    while (server_running) {
        int new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
        if (new_socket < 0) {
            logMessage(logger, "ERROR", "Accept failed");
            perror("Accept failed");
            continue;
        }

        logMessage(logger, "INFO", "Accepted connection from client socket: %d", new_socket);

        // Allocate memory for the ClientTask
        ClientTask* task = malloc(sizeof(ClientTask));
        if (task == NULL) {
            logMessage(logger, "ERROR", "Failed to allocate memory for client task");
            close(new_socket);
            continue;
        }

        task->client_socket = new_socket;
        task->context = &context; // Pass the shared ServerContext

        // Add the task to the thread pool
        if (addTaskToThreadPool(pool, handleClient, task) != 0) {
            logMessage(logger, "ERROR", "Thread pool queue is full. Dropping connection for client socket: %d", new_socket);
            close(new_socket);
            free(task);
        }
    }

    // Cleanup
    // never reached. needs a way to exit the previus infinite loop
    logMessage(logger, "INFO", "Shutting down DNS server");
    destroyLogger(logger);
    destroyThreadPool(pool);
    free(root);
    free(cache);
    close(server_fd);
    return 0;
}
