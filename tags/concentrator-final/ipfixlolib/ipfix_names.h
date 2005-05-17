#ifndef IPFIX_NAMES_H
#define IPFIX_NAMES_H

#include <stdint.h>

#define PROTO_ICMP 1
#define PROTO_IGMP 2
#define PROTO_TCP 6
#define PROTO_UDP 17
/* ??? */
#define PROTO_RAW 255

#define IPFIX_ENTERPRISE_FLAG (1 << 15)

struct ipfix_identifier {
	char *name;
	uint16_t id;
        uint8_t length;
};

struct ipfix_identifier * ipfix_id_lookup(int n);
int ipfix_name_lookup(char *name);

#endif

