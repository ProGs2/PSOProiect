#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "trie.h"
#include <time.h>

#define ROOT_LABEL "root"
#define GET_NR_ZONES_FOR_ROOT_START "./get_nr_zones_for_root_start.sh BINDzones/zones.conf"
#define GET_ARRAY_OF_DOMAINS "./get_array_of_domains.sh BINDzones/zones.conf"
#define GET_PATH_TO_ZONE_FILE "./get_path_to_zone_file.sh BINDzones/zones.conf " // + domain names
#define GET_METADATA_FROM_SOA "./get_metadata_from_soa.sh " // + path_to_file + type
#define GET_NS_DOMAINS "./get_ns_domains.sh " // + path_to_file + type
#define GET_TERMINAL_NAMES "./get_terminal_names.sh " // + path_to_file
#define GET_DNSTYPE "./get_DNStype.sh " // + path_to_file + nr_line 
#define GET_DNSVALUE "./get_DNSvalue.sh " // + path_to_file + nr_line
#define GET_DNSTTL "./get_DNSttl.sh " // + path_to_file + nr_line

void error(char* text)
{
    printf("Error:%s\n", text);
    exit(1);
}
struct TrieNode* createTrieROOT() {
    struct TrieNode* root = (struct TrieNode*)malloc(sizeof(struct TrieNode));
    if (root == NULL) {
        error("Memory allocation failed for root node!");
    }

    root->label = (char*)malloc((strlen(ROOT_LABEL) + 1) * sizeof(char));
    if (root->label == NULL) {
        free(root);
        error("Memory allocation failed for root->label component!");
    }
    strcpy(root->label, ROOT_LABEL);

    for (int i = 0; i < NR_MAX_CHILDREN; i++) {
        root->childrens[i] = NULL;
    }
    root->nr_records = 0;
    root->ns = NULL;
    root->records = NULL;
    root->soa = NULL;

    char buffer[128];
    FILE* fp = popen(GET_NR_ZONES_FOR_ROOT_START, "r");
    if (fp == NULL) {
        error("Error opening pipe for popen() call for GET_NR_ZONES_FOR_ROOT_START");
        free(root->label);
        free(root);
    }

    if (fgets(buffer, sizeof(buffer), fp) != NULL) {
        if (strcmp(buffer, "error\n") == 0) {
            free(root->label);
            free(root);
            pclose(fp);
            error("Error retrieving number of zones!");
        }
        root->nr_childrens = atoi(buffer);
    } else {
        free(root->label);
        free(root);
        pclose(fp);
        error("Error reading number of zones from script!");
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
        error("Error popen() with GET_NR_ZONES_FOR_ROOT_START!");
    }

    int nr_zones;
    if (fgets(buffer, sizeof(buffer), fp) != NULL) {
        if(strcmp(buffer, "error") == 0){
            error("Error in geting the number!");
        }
        nr_zones = atoi(buffer);
    }
    else
    {
        error("Error for fgets() in retriving the number of zones!");
    }
    int num = pclose(fp);
    if(num == -1)
    {
        error("Error in closing the pipe from popen() call with GET_NR_ZONES_FOR_ROOT_START");
    }

    fp = popen(GET_ARRAY_OF_DOMAINS, "r");
    if(fp == -1){
        error("Error in opening a pipe with popen() for GET_ARRAY_OF_DOMAINS!");
    }
    if(fgets(buffer, sizeof(buffer), fp) == NULL)
    {
        error("Error in retriving the array of domains from using popen() with GET_ARRAY_OF_DOMAINS");
    }

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
        error("Error in opening the pipe with popen() with GET_NR_ZONES_FOR_ROOT_START!");
    }

    int nr_zones;
    if (fgets(buffer, sizeof(buffer), fp) != NULL) {
        if(strcmp(buffer, "error") == 0){
            error("Error in geting the number of zones using popen() with GET_NR_ZONES_FOR_ROOT_START!");
        }
        nr_zones = atoi(buffer);
    }
    else
    {
        error("Error for retriving the number of zones with fgets()!");
    }
    int num = pclose(fp);
    if(num == -1)
    {
        error("Error in closing the pipe from popen() with GET_NR_ZONES_FOR_ROOT_START");
    }
    
    return nr_zones;
}
char** extractWordsFromDomain(const char* domain) {
    if (domain == NULL) {
        error("The doamin is NULL!");
    }

    int word_count = 0;
    for (const char* p = domain; *p; p++) {
        if (*p == '.') {
            word_count++;
        }
    }
    word_count++;

    char** words = (char**)malloc((word_count + 1) * sizeof(char*));

    int i = 0;
    char* domain_copy = strdup(domain);
    if (!domain_copy) {
        free(words);
        error("Memory allocation failed for domain_copy");
    }

    char* token = strtok(domain_copy, ".");
    while (token) {
        words[i] = strdup(token);
        if (!words[i]) {
            free(words);
            free(domain_copy);
            error("Memory allocation failed for word");
            for (int j = 0; j < i; j++) {
                free(words[j]);
            }
        }
        i++;
        token = strtok(NULL, ".");
    }
    words[i] = NULL;
    free(domain_copy);

    return words;
}
int getCharArraySize(char** array) {
    if (array == NULL) {
        error("The domain is NULL!");
    }

    int count = 0;
    while (array[count] != NULL) {
        count++;
    }
    return count;
}
struct TrieNode* createNormalNode(char* name)
{
    //perfect:)
    struct TrieNode* node = (struct TrieNode*)malloc(sizeof(struct TrieNode));

    node->label = (char*)malloc(strlen(name) * sizeof(char*));
    strcpy(node->label, name);

    node->nr_records = 0;
    node->ns = NULL;
    node->records = NULL;
    node->soa = NULL;
    node->nr_childrens = 0;

    return node;
}
int getMetadataNumber(char* path_to_zone, char* type)
{
    char buffer[128];
    FILE* fp;
    char* str = (char*)malloc((strlen(path_to_zone) + 2 + strlen(GET_METADATA_FROM_SOA) + 2 + strlen(type) + 1) * sizeof(char));
    strcpy(str, GET_METADATA_FROM_SOA);
    strcat(str, path_to_zone);
    strcat(str, type);
    //printf("-->%s<--\n", str);
    fp = popen(str, "r");
    if(fp == -1){
        error("Error for popen() for GET_METADATA_FROM_SOA");
    }

    if(fgets(buffer, sizeof(buffer), fp) == NULL)
    {
        error("Error retriving the metadata number with fgets() for GET_METADATA_FROM_SOA");
    }

    int num = pclose(fp);
    if(num == -1)
    {
        error("Error in closing the pipe for GET_METADATA_FROM_SOA");
    }

    return atoi(buffer);
}
char* getNSDomain(char* path_to_zone, char* type)
{
    char buffer[128];
    FILE* fp;
    char* str = (char*)malloc((strlen(path_to_zone) + 2 + strlen(GET_NS_DOMAINS) + 2 + strlen(type) + 1) * sizeof(char));

    strcpy(str, GET_NS_DOMAINS);
    strcat(str, path_to_zone);
    strcat(str, type);
    //printf("-->%s<--\n", str);

    fp = popen(str, "r");
    if (fp == NULL) {
        error("Error creating a pipe with popen() for GET_NS_DOMAINS!");
    }

    if(fgets(buffer, sizeof(buffer), fp) == NULL)
    {
        error("Error getting the ns domains with fgets()!");
    }
    if(pclose(fp) == -1)
    {
        error("Error closing the pipe for popen() with GET_NS_DOMAINS");
    }

    buffer[strcspn(buffer, "\n")] = '\0'; //trick

    char* result = (char*)malloc(strlen(buffer) + 1);
    strcpy(result, buffer);

    return result;
}
struct TrieNode* createItselfNode(char* name, char* path_to_zone)
{
    struct TrieNode* node = (struct TrieNode*)malloc(sizeof(struct TrieNode));

    node->label = (char*)malloc(strlen(name) * sizeof(char*));
    strcpy(node->label, name);

    node->nr_records = 0;
    node->nr_childrens = 0;
    node->records = NULL;
    
    node->soa = (struct SOAMetadata*)malloc(sizeof(struct SOAMetadata));
    node->soa->serial_number = getMetadataNumber(path_to_zone, " serial");
    node->soa->refresh_time = getMetadataNumber(path_to_zone, " refresh");
    node->soa->retry_time = getMetadataNumber(path_to_zone, " retry");
    node->soa->expire_time= getMetadataNumber(path_to_zone, " expire");
    node->soa->minimum_ttl = getMetadataNumber(path_to_zone, " minimum_ttl");

    node->ns = (struct NSQuerys*)malloc(sizeof(struct NSQuerys));
    node->ns->domain1 = (char*)malloc(32 * sizeof(char));
    node->ns->domain2 = (char*)malloc(32 * sizeof(char));
    node->ns->domain1 = getNSDomain(path_to_zone, " ns1");
    node->ns->domain2 = getNSDomain(path_to_zone, " ns2");
    //printf("%s\n", node->ns->domain1);
    //node->nr_records++;

    return node;
}
char** getTerminalNames(char* path_to_zone)
{
    char buffer[128];
    FILE* fp;
    char* str = (char*)malloc((strlen(path_to_zone) + 2 + strlen(GET_TERMINAL_NAMES) + 2) * sizeof(char));

    strcpy(str, GET_TERMINAL_NAMES);
    strcat(str, path_to_zone);
    printf("-->%s<--\n", str);

    fp = popen(str, "r");
    if (fp == NULL) {
        error("Error popen for GET_TERMINAL_NAMES!");
    }

    fgets(buffer, sizeof(buffer), fp);
    pclose(fp);
    
    //printf("%s\n", buffer);

    //trebuie sa returnez un array de string-uri :>
    char** names = (char**)malloc(10 * sizeof(char*));
    int index = 0;
    char* token = strtok(buffer, " ");
    while(token != NULL)
    {
        //printf("%s\n", token);
        names[index] = (char*)malloc((strlen(token) + 1) * sizeof(char));
        strcpy(names[index++], token);
        token = strtok(NULL, " ");
    }
    names[index] = NULL;

    return names;
}
char* getDNSTypeFromZoneFile(char* path_to_zone, char* nr_line_in_zone)
{
    char buffer[128];
    FILE* pfile;
    char* str = (char*)malloc((strlen(GET_DNSTYPE) + 2 + strlen(path_to_zone) + strlen(nr_line_in_zone) + 1) * sizeof(char));
    strcpy(str, GET_DNSTYPE);
    strcat(str, path_to_zone);
    strcat(str, nr_line_in_zone);
    printf("-->%s<--\n", str);
    pfile = popen(str, "r");

    fgets(buffer, sizeof(buffer), pfile);
    pclose(pfile);

    char* result = malloc(strlen(buffer) + 1);
    strcpy(result, buffer);

    return result;
}
char* getDNSValueFromZoneFile(char* path_to_zone, char* nr_line_in_zone)
{
    char buffer[128];
    FILE* pfile;
    char* str = (char*)malloc((strlen(GET_DNSVALUE) + 2 + strlen(path_to_zone) + strlen(nr_line_in_zone) + 1) * sizeof(char));
    strcpy(str, GET_DNSVALUE);
    strcat(str, path_to_zone);
    strcat(str, nr_line_in_zone);
    printf("-->%s<--\n", str);
    pfile = popen(str, "r");

    fgets(buffer, sizeof(buffer), pfile);
    pclose(pfile);

    char* result = malloc(strlen(buffer) + 1);
    strcpy(result, buffer);

    return result;
}
int getDNSTtlValueFromZoneFile(char* path_to_zone, char* nr_line_in_zone)
{
    char buffer[128];
    FILE* pfile;
    char* str = (char*)malloc((strlen(GET_DNSTTL) + 2 + strlen(path_to_zone) + strlen(nr_line_in_zone) + 1) * sizeof(char));
    strcpy(str, GET_DNSTTL);
    strcat(str, path_to_zone);
    strcat(str, nr_line_in_zone);
    printf("-->%s<--\n", str);
    pfile = popen(str, "r");

    fgets(buffer, sizeof(buffer), pfile);
    pclose(pfile);

    return atoi(buffer);
}
struct TrieNode* createSpecialNode(char* path_to_zone, int nr_line_in_zone, char* name)
{
    struct TrieNode* node = (struct TrieNode*)malloc(sizeof(struct TrieNode));

    node->label = (char*)malloc((strlen(name) + 1) * sizeof(char));
    strcpy(node->label, name);
    //printf("%s\n", name);
    node->nr_childrens = 0;
    node->nr_records = 0; //deocamdata
    node->ns = NULL;
    node->soa = NULL;
    
    node->records = (struct DNSRecord*)malloc(sizeof(struct DNSRecord));

    char str_number[5];
    sprintf(str_number, "%d", nr_line_in_zone);
    char spaceCh[] = " ";
    char* string = (char*)malloc((strlen(str_number) + 2 + strlen(spaceCh)) * sizeof(char));
    strcpy(string, spaceCh);
    strcat(string, str_number);

    node->records->type = getDNSTypeFromZoneFile(path_to_zone, string);
    node->records->value = getDNSValueFromZoneFile(path_to_zone, string);
    node->records->ttl = getDNSTtlValueFromZoneFile(path_to_zone, " 1");
    node->nr_records++;

    return node;
}
struct TrieNode* createTerminalNode(char* name, char* domain)
{
    //find the file path to the zone file
    struct TrieNode* terminalNode = (struct TrieNode*)malloc(sizeof(struct TrieNode));
    terminalNode->nr_childrens = 0;
    terminalNode->nr_records = 0;
    terminalNode->records = NULL;

    terminalNode->label = (char*)malloc((strlen(name) + 1) * sizeof(char));
    strcpy(terminalNode->label, name);

    char buffer[128];
    FILE* fp;
    char* str = (char*)malloc((strlen(GET_PATH_TO_ZONE_FILE) + 2 + strlen(name)) * sizeof(char));
    strcpy(str, GET_PATH_TO_ZONE_FILE);
    strcat(str, domain);
    fp = popen(str, "r");

    fgets(buffer, sizeof(buffer), fp);
    printf("The path file of the zone: %s\n", buffer);
    buffer[strcspn(buffer, "\n")] = '\0';   ///succes :)

    pclose(fp);

    //prelucrate the zone file//////////////////////////////////
    //@(itself node)
    struct TrieNode* itselfNode = createItselfNode("@", buffer);
    terminalNode->childrens[terminalNode->nr_childrens++] = itselfNode; // add itselfNode
    //prelucrate specialNodes
    char** terminal_names = getTerminalNames(buffer);
    int index = 0;
    int start_index = 11;
    while(terminal_names[index] != NULL)
    {
        struct TrieNode* specialNode = createSpecialNode(buffer, start_index, terminal_names[index]);
        terminalNode->childrens[terminalNode->nr_childrens++] = specialNode;
        index++; //deocamdata
        start_index++;
    }
    //verificare
    //printf("%s\n", terminalNode->childrens[3]->label);

    //if(terminalNode == NULL){printf("error");}

    return terminalNode;
}
struct TrieNode* createBranch(char* domain)
{
    //int nr_domains = getNrBranches();
    //for(int i=0;i<nr_domains;i++)
    //{
        char** names = extractWordsFromDomain(domain);
        int nr_names = getCharArraySize(names);  //cu -1
        //names contains for exemple : ["exemple", "com"]
        struct TrieNode** vectorOfNodes = (struct TrieNode**)malloc(sizeof(struct TrieNode*) * (nr_names + 1));
        int index = 0;
        //printf("%d\n", nr_names);
        for(int j = nr_names - 1;j >= 0;j--)
        {
            if(j != 0) // normalNode
            {
                struct TrieNode* normalNode = createNormalNode(names[j]);
                vectorOfNodes[index] = normalNode;
            }
            else //terminalNode that has multiple specialNodes
            {
                //printf("%s\n", names[j]);
                struct TrieNode* terminalNode = createTerminalNode(names[j], domain);
                vectorOfNodes[index] = terminalNode;
            }
            index++;
        }

        // printf("Branch:\n");
        // for(int i=0;i<=nr_names - 1;i++)
        // {
        //     printf("%s --- ", vectorOfNodes[i]->label); // wow
        // }

        for(int i=1;i <= nr_names - 1;i++)
        {
            vectorOfNodes[0]->childrens[vectorOfNodes[0]->nr_childrens++] = vectorOfNodes[i];
        }
    //}
    return vectorOfNodes[0];
}
char* retriveValue(struct TrieNode* root, char* domain_name)
{
    char** words = extractWordsFromDomain(domain_name);
    int nr_words = getCharArraySize(words) - 1;
    for(int i=0;i<nr_words + 1;i++)
    {
        printf("-->%s<--\n", words[i]);
    }
    struct TrieNode* search_node = (struct TrieNode*)malloc(sizeof(struct TrieNode*));
    search_node = root;
    while(1)
    {
        for(int i=0;i<search_node->nr_childrens;i++)
        {
            printf("%s are %d copii\n", search_node->label, search_node->nr_childrens);
            printf("%s == %s\n", search_node->childrens[i]->label, words[nr_words]);
            printf("%d\n", nr_words);
            if(strcmp(search_node->childrens[i]->label, words[nr_words]) == 0){
                nr_words--;
                printf("%s == %s\n", search_node->childrens[i]->label, words[nr_words + 1]);
                search_node = search_node->childrens[i];
                i = -1; //reset
                for(int j=0;j<search_node->nr_records;j++)
                {
                    //printf("%s\n", search_node->records[i].value);
                    if(search_node->records[j].value != NULL && nr_words == -1)
                    {
                        printf("%s\n", search_node->records[j].value);
                        return search_node->records[j].value;
                    }
                }
                if(nr_words == -1 ){
                    search_node = search_node->childrens[0];
                    printf("Itself node found!\n");
                    printf("%d %s\n", search_node->nr_records, search_node->label);
                    srand(time(0));
                    int rand_nr = rand() % 2;
                    //printf("%d\n", rand_nr);
                    if(rand_nr == 0)
                    {
                        return retriveValue(root, search_node->ns->domain1);
                    }
                    else{
                        return retriveValue(root, search_node->ns->domain2);
                    }
                }
            }
        }
        return "Failed to find the value!";
    }
}