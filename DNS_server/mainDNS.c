#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "trie.h"
#include "cache.h"

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

int main(int argc, char* argv[])
{
    //The creation of the DNS trie structure;
    struct TrieNode* root = createTrieROOT();

    char** domains = getArrayOfDomainNames();

    int nr_branches = getNrBranches();
    for(int i=0;i<nr_branches;i++)
    {
        struct TrieNode* branch = createBranch(domains[i]);
        //printf("%s\n", branch->childrens[0]->childrens[0]->label);
        root->childrens[i] = branch;
    }
    
    printf("Structura Trie:\n");
    printTrie(root, 0);

    //initializare cache
    struct DNSCache* cache = initializeDNSCache();
    struct CacheEntry* cache_entry = (struct CacheEntry*)malloc(sizeof(struct CacheEntry*));
    while(1)
    {
        cache_entry = retriveValue(root, argv[1], cache);
        printf("%s\n", cache_entry->record_value);
        cache = addCacheEntry(cache, cache_entry);
    }

    //free(cache);
    free(cache_entry);

    return 0;
}