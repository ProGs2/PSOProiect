#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "trie.h"

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

int main()
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

    return 0;
}