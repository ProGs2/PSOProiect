#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "trie.h"

int main()
{
    //The creation of the DNS trie structure;
    struct TrieNode* root = createTrieROOT();
    char** domains = getArrayOfDomainNames();

    int nr_branches = getNrBranches();
    for(int i=0;i<nr_branches;i++)
    {
        struct TrieNode* branch = createBranch(domains);
        
    }
    return 0;
}