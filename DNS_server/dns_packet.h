#ifndef __DNS_PACKET_H__
#define __DNS_PACKET_H__

#include <sys/types.h>

#define QR_QUERY 0 /* message is a question */
#define QR_RESPONSE 1 /* message is an answer */

#define OPCODE_QUERY 0 /* a standard query */
#define OPCODE_IQUERY 1 /* an inverse query */
#define OPCODE_STATUS 2 /* a server status request */

#define AA_NONAUTHORITY 0 /* used when message is sent from anything that is NOT the authoritative server */
#define AA_AUTHORITY 1 /* used when message is sent from the authoritative server */

struct dns_header
{
	u_int16_t id; /* 16 bit identifier */
	u_int16_t qr:1;
	u_int16_t opcode:4;
	u_int16_t aa:1;
	u_int16_t tc:1;
	u_int16_t rd:1;
	u_int16_t ra:1;
	u_int16_t z:3;
	u_int16_t rcode:4;
	u_int16_t qdcount;
	u_int16_t ancount;
	u_int16_t nscount;
	u_int16_t arcount;
};

struct dns_question
{
	char *qname;
	u_int16_t qtype;
	u_int16_t qclass;
};

struct dns_answer
{
	char *name;
	u_int16_t type;
	u_int16_t class;
	u_int32_t ttl;
	u_int16_t rdlength;
	char *rdata;
};

struct dns_packet
{
	struct dns_header header;
	struct dns_question question;
	struct dns_answer answer;
//	struct dns_authority authority;
//	struct dns_additional additional;
};

void dns_print_packet (const struct dns_packet *packet);
void dns_print_header (const struct dns_header *header);
void dns_print_question (const struct dns_question *question);
void dns_print_answer (const struct dns_answer *answer);

int dns_request_parse (struct dns_packet *pkt, const void *data);
int dns_header_parse (struct dns_header *header, const void *data);
int dns_question_parse (struct dns_question *question, const void *data);
int dns_answer_parse (struct dns_answer *answer, const void *data);

#endif
