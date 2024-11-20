#include <arpa/inet.h>

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <string.h>

#include "dns_packet.h"

size_t util_measure_name(void* data, u_int16_t offset)
{
	size_t length = 0;

	while(data[length] != '0') {
		length++;
	}

	return length;
}

void dns_print_packet (const struct dns_packet *packet)
{
	printf("Printing packet data...\n\n");

	dns_print_header (&packet->header);

	dns_print_question (&packet->question);

	dns_print_answer (&packet->answer);
}

void dns_print_header (const struct dns_header *header)
{
	printf ("ID: %d\n", header->id);
	printf ("qr: %d\n", header->qr);
	printf ("opcode: %d\n", header->opcode);
	printf ("aa: %d\n", header->aa);
	printf ("tc: %d\n", header->tc);
	printf ("rd: %d\n", header->rd);
	printf ("ra: %d\n", header->ra);
	printf ("z: %d\n", header->z);
	printf ("rcode: %d\n", header->rcode);

	printf ("qdcount: %d\n", header->qdcount);
	printf ("ancount: %d\n", header->ancount);
	printf ("nscount: %d\n", header->nscount);
	printf ("arcount: %d\n", header->arcount);
}

void dns_print_question (const struct dns_question *question)
{
	printf("qname: %s\n", question->qname);

	printf("qtype: %d\n", question->qtype);
	printf("qclass: %d\n", question->qclass);
}

void dns_print_answer (const struct dns_answer *answer)
{
	printf("name: %s", answer->name);

	printf("type: %d\n", answer->type);
	printf("class: %d\n", answer->class);
	printf("ttl: %d\n", answer->ttl);
	
	printf("rdlength: %d\n", answer->rdlength);
	printf("rdata: %s", answer->rdata);
}

int dns_request_parse (struct dns_packet *pkt, const void *data)
{
	int i;

	dns_header_parse (&pkt->header, data);

/* TODO: Implement parser function for question */
	dns_question_parse (&pkt->question, data);

/* TODO: implement parses function for answer */
	dns_answer_parse (&pkt->answer, data);

	return EXIT_SUCCESS;
}

int dns_header_parse (struct dns_header *header, const void *data)
{
	memcpy (header, data, 12);

	header->id = ntohs (header->id);
	header->qdcount = ntohs (header->qdcount);
	header->ancount = ntohs (header->ancount);
	header->nscount = ntohs (header->nscount);
	header->arcount = ntohs (header->arcount);

	return EXIT_SUCCESS;
}

int dns_question_parse (struct dns_question *question, const void *data)
{
	/* TODO: implementation */

	memccpy (question, data + 12, )
	
	u_int16_t i = 0;
	

	return EXIT_SUCCESS;
}

int dns_answer_parse (struct dns_answer *answer, const void *data)
{
	/* TODO: implementation */

	return EXIT_SUCCESS;
}