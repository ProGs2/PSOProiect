#ifndef __DNS_PACKET_H__
#define __DNS_PACKET_H__

#include <sys/types.h>
#include <netinet/in.h>

/* header flags */
#define QR_QUERY 0			/* message is a question */
#define QR_RESPONSE 1		/* message is an answer */

#define OPCODE_QUERY 0		/* a standard query */
#define OPCODE_IQUERY 1		/* an inverse query */
#define OPCODE_STATUS 2		/* a server status request */

#define AA_NONAUTHORITY 0	/* used when message is sent from anything that is NOT the authoritative server */
#define AA_AUTHORITY 1 		/* used when message is sent from the authoritative server */

/* record types */
#define DNS_TYPE_A 1      	/* host address */
#define DNS_TYPE_NS 2     	/* authoritative name server */
#define DNS_TYPE_CNAME 5  	/* canonical name for alias */
#define DNS_TYPE_SOA 6    	/* start of zone of authority */
#define DNS_TYPE_PTR 12   	/* domain name pointer */
#define DNS_TYPE_MX 15    	/* mail exchange */
#define DNS_TYPE_TXT 16   	/* text strings */

#define DNS_CLASS_IN 1    /* dns internet class */

struct dns_header {
    uint16_t id;      /* query id */
    #if __BYTE_ORDER == __LITTLE_ENDIAN
    uint16_t rd:1;    /* recursion desired */
    uint16_t tc:1;    /* truncated message */
    uint16_t aa:1;    /* authoritative answer */
    uint16_t opcode:4;/* purpose of message */
    uint16_t qr:1;    /* response flag */
    uint16_t rcode:4; /* response code */
    uint16_t cd:1;    /* checking disabled */
    uint16_t ad:1;    /* authenticated data */
    uint16_t z:1;     /* reserved */
    uint16_t ra:1;    /* recursion available */
    #elif __BYTE_ORDER == __BIG_ENDIAN
    uint16_t qr:1;    /* response flag */
    uint16_t opcode:4;/* purpose of message */
    uint16_t aa:1;    /* authoritative answer */
    uint16_t tc:1;    /* truncated message */
    uint16_t rd:1;    /* recursion desired */
    uint16_t ra:1;    /* recursion available */
    uint16_t z:1;     /* reserved */
    uint16_t ad:1;    /* authenticated data */
    uint16_t cd:1;    /* checking disabled */
    uint16_t rcode:4; /* response code */
    #endif
    uint16_t qdcount;/* number of question entries */
    uint16_t ancount;/* number of answer entries */
    uint16_t nscount;/* number of authority entries */
    uint16_t arcount;/* number of resource entries */
};

struct dns_question
{
	char *qname;
	uint16_t qtype;
	uint16_t qclass;
};

struct dns_answer
{
	char *name;
	uint16_t type;
	uint16_t class;
	uint32_t ttl;
	uint16_t rdlength;
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

/* functions */
/* parsing */
int dns_request_parse(struct dns_packet *pkt, const void *data);
int dns_header_parse(struct dns_header *header, const void *data);
int dns_question_parse(struct dns_question *question, const void *data, size_t* offset);
int dns_answer_parse(struct dns_answer *answer, const void *data);

/* printing (debug) */
void dns_print_packet(const struct dns_packet *packet);
void dns_print_header(const struct dns_header *header);
void dns_print_question(const struct dns_question *question);
void dns_print_answer(const struct dns_answer *answer);

/* utility */
size_t util_measure_name(const void *data, uint16_t offset);
int dns_read_name(char *dest, const void *data, uint16_t offset, size_t max_len);
struct dns_packet* dns_create_query_packet(const void* in_qname);
void dns_free_packet(struct dns_packet* packet);

#endif
