#include <stdio.h>
#include "DNS1.h"

#define MAX_TRIES 4

DNSMessageReader getResponse(struct ComboAddress* server, struct DNSName* dn, struct DNSType* dt, int depth)
{
    int doEDNS = 1;
    int doTCP = 0;
    DNSMessageWriter dmw = DNSMessageWriter_init(dn, dt);
    dmw.dh.rd = 0;
    DNSMessageReader dmr;
    
    for (int tries = 0; tries < MAX_TRIES; ++tries) {
        if (++d_numqueries > d_maxqueries) {
            printf("Too many queries\n");
            return DNSMessageReader_init(); // Too many queries, exit
        }

        // Prepare the message writer
        dmw_randomizeID(&dmw);
        if (doEDNS) {
            dmw_setEDNS(&dmw, 1500, 0); // No DNSSEC, buffer size 1500
        }

        char resp[65535];
        double timeout = 1.0;
        int err = 0;
        
        if (doTCP) {
            // TCP handling
            int sock = socket(server->sin_family, SOCK_STREAM, 0);
            SConnect(sock, server);

            uint16_t len = htons(dmw_length(&dmw));
            SWrite(sock, (char*)&len, sizeof(len));
            SWrite(sock, dmw_serialize(&dmw), dmw_length(&dmw));

            err = waitForData(sock, &timeout);
            if (err <= 0) {
                incrementSkipCount(server, dn, dt);
                close(sock);
                printf("Error waiting for TCP data: %s\n", (err ? strerror(errno) : "Timeout"));
                continue;
            }
            
            SRead(sock, resp, sizeof(resp));
            dmr = DNSMessageReader_init(resp);
            close(sock);
        } else {
            // UDP handling
            int sock = socket(server->sin_family, SOCK_DGRAM, 0);
            SConnect(sock, server);

            SWrite(sock, dmw_serialize(&dmw), dmw_length(&dmw));
            err = waitForData(sock, &timeout);
            if (err <= 0) {
                incrementSkipCount(server, dn, dt);
                close(sock);
                printf("Error waiting for UDP data: %s\n", (err ? strerror(errno) : "Timeout"));
                continue;
            }

            SRecvfrom(sock, resp, sizeof(resp), NULL);
            dmr = DNSMessageReader_init(resp);
            close(sock);
        }

        // Reset the skip counter since the query succeeded
        resetSkipCount(server, dn, dt);

        if (dmr.dh.id != dmw.dh.id) {
            printf("ID mismatch on answer\n");
            continue;
        }
        if (!dmr.dh.qr) {
            printf("Received non-response packet\n");
            continue;
        }
        if (dmr.dh.rcode == RCODE_FORMERR) {
            printf("Form error, resending without EDNS\n");
            doEDNS = 0;
            continue;
        }
        if (dmr.dh.tc) {
            printf("Truncated answer, retrying over TCP\n");
            doTCP = 1;
            continue;
        }

        return dmr;
    }

    return DNSMessageReader_init(); // Return empty reader as error signal if all tries fail
}