#include <stdio.h>
#include <string.h>
#include "DNS1.h"

int main() {
    // Define the hints array with root servers
    //avem un starting point, the backbone of DNS hierarchy
    DNSHint hints[MAX_HINTS] = {
        {makeDNSName("a.root-servers.net"), {"198.41.0.4", 53}},
        {makeDNSName("f.root-servers.net"), {"192.5.5.241", 53}},
        {makeDNSName("k.root-servers.net"), {"193.0.14.129", 53}}
    };
    printf("Hints initialized succesfully!\n");

    

    return 0;
}
