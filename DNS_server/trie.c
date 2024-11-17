#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "trie.h"

#define DEFAULT_LABEL ""
#define GET_NR_ZONES_FOR_ROOT_START "./get_nr_zones_for_root_start.sh BINDzones/zones.conf"
#define GET_ARRAY_OF_DOMAINS "./get_array_of_domains.sh BINDzones/zones.conf"

void error(char* text)
{
    printf("%s\n", text);
}
struct TrieNode* createTrieROOT() {
    struct TrieNode* root = (struct TrieNode*)malloc(sizeof(struct TrieNode));
    if (!root) {
        error("Memory allocation failed for root node!");
        exit(1);
    }

    root->label = (char*)malloc(strlen(DEFAULT_LABEL) + 1);
    if (!root->label) {
        free(root);
        error("Memory allocation failed for root label!");
        exit(1);
    }
    strcpy(root->label, DEFAULT_LABEL);

    for (int i = 0; i < NR_MAX_CHILDREN; i++) {
        root->childrens[i] = NULL;
    }
    root->nr_records = 0;
    root->ns = NULL;
    root->records = NULL;
    root->soa = NULL;

    char buffer[128];
    FILE* fp = popen(GET_NR_ZONES_FOR_ROOT_START, "r");
    if (!fp) {
        error("Error opening pipe for GET_NR_ZONES_FOR_ROOT_START!");
        free(root->label);
        free(root);
        exit(1);
    }

    if (fgets(buffer, sizeof(buffer), fp) != NULL) {
        if (strcmp(buffer, "error\n") == 0) {
            error("Error retrieving number of zones!");
            free(root->label);
            free(root);
            pclose(fp);
            exit(1);
        }
        root->nr_childrens = atoi(buffer);
    } else {
        error("Error reading number of zones from script!");
        free(root->label);
        free(root);
        pclose(fp);
        exit(1);
    }
    pclose(fp);

    return root;
}

char** getArrayOfDomainNames()
{
    char buffer[128];
    FILE* fp;
    fp = popen(GET_NR_ZONES_FOR_ROOT_START, "r");
    if(fp == -1){
        error("Error popen!");
        exit(1);
    }

    int nr_zones;
    if (fgets(buffer, sizeof(buffer), fp) != NULL) {
        if(strcmp(buffer, "error") == 0){
            error("Error in geting the number!");
            exit(1);
        }
        nr_zones = atoi(buffer);
    }
    else
    {
        error("Error for fgets!");
        exit(1);
    }
    fclose(fp);

    fp = popen(GET_ARRAY_OF_DOMAINS, "r");
    if(fp == -1){
        error("Error popen!");
        exit(1);
    }
    fgets(buffer, sizeof(buffer), fp);

    char** domains = (char**)malloc(nr_zones * sizeof(char*));
    for(int i=0;i<nr_zones;i++)
    {
        char delim[] = " \n";
        char* token = strtok(buffer, delim);
        while(token != NULL)
        {
            domains[i] = (char*)malloc(strlen(token) * sizeof(char));
            strcpy(domains[i], token);
            token = strtok(NULL, delim);
        }
    }
    printf("The domain names are:\n");
    for(int i=0;i<nr_zones;i++)
    {
        printf("-->%s<--\n", domains[i]);
    }
    return domains;
}
int getNrBranches()
{
    char buffer[128];
    FILE* fp;
    fp = popen(GET_NR_ZONES_FOR_ROOT_START, "r");
    if(fp == -1){
        error("Error popen!");
        exit(1);
    }

    int nr_zones;
    if (fgets(buffer, sizeof(buffer), fp) != NULL) {
        if(strcmp(buffer, "error") == 0){
            error("Error in geting the number!");
            exit(1);
        }
        nr_zones = atoi(buffer);
    }
    else
    {
        error("Error for fgets!");
        exit(1);
    }
    fclose(fp);
    
    return nr_zones;
}
char** extractWordsFromDomain(const char* domain) {
    if (!domain) {
        return NULL;
    }

    int word_count = 0;
    for (const char* p = domain; *p; p++) {
        if (*p == '.') {
            word_count++;
        }
    }
    word_count++;

    char** words = (char**)malloc((word_count + 1) * sizeof(char*));
    if (!words) {
        perror("Memory allocation failed");
        exit(1);
    }

    int i = 0;
    char* domain_copy = strdup(domain);
    if (!domain_copy) {
        perror("Memory allocation failed for domain copy");
        free(words);
        exit(1);
    }

    char* token = strtok(domain_copy, ".");
    while (token) {
        words[i] = strdup(token);
        if (!words[i]) {
            perror("Memory allocation failed for word");
            for (int j = 0; j < i; j++) {
                free(words[j]);
            }
            free(words);
            free(domain_copy);
            exit(1);
        }
        i++;
        token = strtok(NULL, ".");
    }
    words[i] = NULL;

    free(domain_copy);
    return words;
}
int getCharArraySize(char** array) {
    if (!array) {
        return 0;
    }

    int count = 0;
    while (array[count] != NULL) {
        count++;
    }
    return count;
}
struct TrieNode* createNormalNode(char* name)
{
    struct TrieNode* node = (struct TrieNode*)malloc(sizeof(struct TrieNode));

    node->label = (char*)malloc(strlen(name) * sizeof(char*));
    strcpy(node->label, name);
    node->nr_records = 0;
    node->ns = NULL;
    node->records = NULL;
    node->soa = NULL;

    return node;
}
struct TrieNode* createBranch(char** domains)
{
    int nr_domains = getNrBranches();
    for(int i=0;i<nr_domains;i++)
    {
        char** names = extractWordsFromDomain(domains[i]);
        int nr_names = getCharArraySize(names);  //cu -1
        //probabil trebe ceva vector de noduri normale si terminale
        //create nodes :)
        for(int j = nr_names - 1;j >= 0;j--)
        {
            if(j!=0) // normalNode
            {

            }
            else
            {

            }
        }

    }
}