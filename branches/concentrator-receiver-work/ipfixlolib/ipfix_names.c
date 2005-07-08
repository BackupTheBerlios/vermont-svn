#include <string.h>
#include "ipfix_names.h"

static struct ipfix_identifier IDTAB[] = {
	{ "RESERVED", 0, 0 },
	{ "inOctetDeltaCount", 1, 8 },
        { "inPacketDeltaCount", 2, 8 },
        { "totalFlowCount", 3, 8 },
        { "protocolIdentifier", 4, 1 },
        { "classOfServiceIPv4", 5, 1 },
        { "tcpControlBits", 6, 1 },
        { "sourceTransportPort", 7, 2 },
        { "sourceIPv4Address", 8, 4 },
        { "sourceIPv4Mask", 9, 1 },
        { "ingressInterface", 10, 4 },
        { "destinationTransportPort", 11, 0 },
        { "destinationIPv4Address", 12, 4 },
        { "destinationIPv4Mask", 13, 1 },
        { "egressInterface", 14, 4 },
        { "ipNextHopIPv4Address", 15, 4 },
        { "bgpSourceAsNumber", 16, 2 },
        { "bgpDestinationAsNumber", 17, 2 },
        { "bgpNextHopIPv4Address", 18, 4 },
        { "OutMulticastPacketCount", 19, 8 },
        { "OutMulticastOctetCount", 20, 8 },
        { "flowEndTime", 21, 4 },
        { "flowCreationTime", 22, 4 },
        { "outOctetDeltaCount", 23, 8 },
        { "outPacketDeltaCount", 24, 8 },
        { "minimumPacketLength", 25, 2 },
        { "maximumPacketLength", 26, 2 },
        { "sourceIPv6Address", 27, 8 },
        { "destinationIPv6Address", 28, 8 },
        { "sourceIPv6Mask", 29, 1 },
        { "destinationIPv6Mask", 30, 0 },
        { "flowLabelIPv6", 31, 0 },
        { "icmpTypeCode", 32, 2 },
        { "igmpType", 33, 1 },
        { "RESERVED", 34, 0 },
        { "RESERVED", 35, 0 },
        { "flowActiveTimeOut", 36, 2 },
        { "flowInactiveTimeout", 37, 2 },
        { "RESERVED", 38, 0 },
        { "RESERVED", 39, 0 },
        { "exportedOctetCount", 40, 8 },
        { "exportedPacketCount", 41, 8 },
        { "exportedFlowCount", 42, 8 },
        { "RESERVED", 43, 0 },
        { "sourceIPv4Prefix", 44, 4 },
        { "destinationIPv4Prefix", 45, 0 },
        { "mplsTopLabelType", 46, 1 },
        { "mplsTopLabelIPv4Address", 47, 4 },
        { "RESERVED", 48, 0 },
        { "RESERVED", 49, 0 },
        { "RESERVED", 50, 0 },
        { "RESERVED", 51, 0 },
        { "minimumTTL", 52, 1 },
        { "maximumTTL", 53, 1 },
        { "identificationIPv4", 54, 1 },
        { "RESERVED", 55, 0 },
        { "sourceMacAddress", 56, 1 },
        { "RESERVED", 57, 0 },
        { "RESERVED", 58, 0 },
        { "RESERVED", 59, 0 },
        { "ipVersion", 60, 0 },
        { "RESERVED", 61, 0 },
        { "ipNextHopIPv6Address", 62, 8 },
        { "bgpNextHopIPv6Address", 63, 8 },
        { "ipv6OptionHeaders", 64, 4 },
        { "RESERVED", 65, 0 },
        { "RESERVED", 66, 0 },
        { "RESERVED", 67, 0 },
        { "RESERVED", 68, 0 },
        { "RESERVED", 69, 0 },
        { "mplsLabelStackEntry1", 70, 4 },
        { "mplsLabelStackEntry2", 71, 4 },
        { "mplsLabelStackEntry3", 72, 4 },
        { "mplsLabelStackEntry4", 73, 4 },
        { "mplsLabelStackEntry5", 74, 4 },
        { "mplsLabelStackEntry6", 75, 4 },
        { "mplsLabelStackEntry7", 76, 4 },
        { "mplsLabelStackEntry8", 77, 4 },
        { "mplsLabelStackEntry9", 78, 4 },
        { "mplsLabelStackEntry10", 79, 4 },
        { "RESERVED", 80, 0 },
        { "RESERVED", 81, 0 },
        { "RESERVED", 82, 0 },
        { "RESERVED", 83, 0 },
        { "RESERVED", 84, 0 },
        { "octetTotalCount", 85, 8 },
        { "packetTotalCount", 86, 8 },
        { "RESERVED", 87, 0 },
        { "RESERVED", 88, 0 },
        { "RESERVED", 89, 0 },
        { "RESERVED", 90, 0 },
        { "RESERVED", 91, 0 },
        { "RESERVED", 92, 0 },
        { "RESERVED", 93, 0 },
        { "RESERVED", 94, 0 },
        { "RESERVED", 95, 0 },
        { "RESERVED", 96, 0 },
        { "RESERVED", 97, 0 },
        { "RESERVED", 98, 0 },
        { "RESERVED", 99, 0 },
        { "RESERVED", 100, 0 },
        { "RESERVED", 101, 0 },
        { "RESERVED", 102, 0 },
        { "RESERVED", 103, 0 },
        { "RESERVED", 104, 0 },
        { "RESERVED", 105, 0 },
        { "RESERVED", 106, 0 },
        { "RESERVED", 107, 0 },
        { "RESERVED", 108, 0 },
        { "RESERVED", 109, 0 },
        { "RESERVED", 110, 0 },
        { "RESERVED", 111, 0 },
        { "RESERVED", 112, 0 },
        { "RESERVED", 113, 0 },
        { "RESERVED", 114, 0 },
        { "RESERVED", 115, 0 },
        { "RESERVED", 116, 0 },
        { "RESERVED", 117, 0 },
        { "RESERVED", 118, 0 },
        { "RESERVED", 119, 0 },
        { "RESERVED", 120, 0 },
        { "RESERVED", 121, 0 },
        { "RESERVED", 122, 0 },
        { "RESERVED", 123, 0 },
        { "RESERVED", 124, 0 },
        { "RESERVED", 125, 0 },
        { "RESERVED", 126, 0 },
        { "RESERVED", 127, 0 },
        { "bgpNextHopAsNumber", 128, 2 },
        { "ipNextHopAsNumber", 129, 2 },
        { "exporterIPv4Address", 130, 4 },
        { "exporterIPv6Address", 131, 8 },
        { "droppedOctetDeltaCount", 132, 8 },
        { "droppedPacketDeltaCount", 133, 8 },
        { "droppedOctetTotalCount", 134, 8 },
        { "droppedPacketTotalCount", 135, 8 },
        { "flowEndReason", 136, 1 },
        { "classOfServiceIPv6", 137, 1 },
        { "octetDeltaCount", 138, 8 },
        { "packetDeltaCount", 139, 8 },
        { "mplsTopLabelIPv6Address", 140, 8 }
};


/*
 ANSI-C code produced by gperf version 2.7.2
 Command-line: gperf -D -C -t -L ANSI-C
 */

/* this is an internal and minimal type for name->id mapping, only used by hash lookups */
struct ipfix_midentifier {
	char *name;
	uint16_t id;
};

#define TOTAL_KEYWORDS 140
#define MIN_WORD_LENGTH 8
#define MAX_WORD_LENGTH 24
#define MIN_HASH_VALUE 8
#define MAX_HASH_VALUE 97
/* maximum key range = 90, duplicates = 78 */

static inline unsigned int hash(register const char *str, register unsigned int len)
{
	static const unsigned char asso_values[] = {
		98, 98, 98, 98, 98, 98, 98, 98, 98, 98,
		98, 98, 98, 98, 98, 98, 98, 98, 98, 98,
		98, 98, 98, 98, 98, 98, 98, 98, 98, 98,
		98, 98, 98, 98, 98, 98, 98, 98, 98, 98,
		98, 98, 98, 98, 98, 98, 98, 98, 15, 5,
		50, 25, 10, 30, 20, 35, 40, 0, 98, 98,
		98, 98, 98, 98, 98, 98, 98, 98, 0, 98,
		98, 98, 98, 98, 98, 98, 0, 98, 98, 57,
		98, 98, 0, 98, 98, 98, 98, 98, 98, 98,
		98, 98, 98, 98, 98, 98, 98, 98, 55, 25,
		10, 30, 50, 98, 10, 40, 98, 45, 98, 0,
		10, 20, 5, 98, 20, 5, 0, 98, 98, 98,
		10, 98, 98, 98, 98, 98, 98, 98, 98, 98,
		98, 98, 98, 98, 98, 98, 98, 98, 98, 98,
		98, 98, 98, 98, 98, 98, 98, 98, 98, 98,
		98, 98, 98, 98, 98, 98, 98, 98, 98, 98,
		98, 98, 98, 98, 98, 98, 98, 98, 98, 98,
		98, 98, 98, 98, 98, 98, 98, 98, 98, 98,
		98, 98, 98, 98, 98, 98, 98, 98, 98, 98,
		98, 98, 98, 98, 98, 98, 98, 98, 98, 98,
		98, 98, 98, 98, 98, 98, 98, 98, 98, 98,
		98, 98, 98, 98, 98, 98, 98, 98, 98, 98,
		98, 98, 98, 98, 98, 98, 98, 98, 98, 98,
		98, 98, 98, 98, 98, 98, 98, 98, 98, 98,
		98, 98, 98, 98, 98, 98, 98, 98, 98, 98,
		98, 98, 98, 98, 98, 98
	};
	return len + asso_values[(unsigned char)str[len - 1]] + asso_values[(unsigned char)str[0]];
}


static const struct ipfix_midentifier * in_word_set(const char *str, unsigned int len)
{
	static const struct ipfix_midentifier wordlist[] = {
		{"RESERVED", 34},
		{"RESERVED", 127},
		{"RESERVED", 126},
		{"RESERVED", 125},
		{"RESERVED", 124},
		{"RESERVED", 123},
		{"RESERVED", 122},
		{"RESERVED", 121},
		{"RESERVED", 120},
		{"RESERVED", 119},
		{"RESERVED", 118},
		{"RESERVED", 117},
		{"RESERVED", 116},
		{"RESERVED", 115},
		{"RESERVED", 114},
		{"RESERVED", 113},
		{"RESERVED", 112},
		{"RESERVED", 111},
		{"RESERVED", 110},
		{"RESERVED", 109},
		{"RESERVED", 108},
		{"RESERVED", 107},
		{"RESERVED", 106},
		{"RESERVED", 105},
		{"RESERVED", 104},
		{"RESERVED", 103},
		{"RESERVED", 102},
		{"RESERVED", 101},
		{"RESERVED", 100},
		{"RESERVED", 99},
		{"RESERVED", 98},
		{"RESERVED", 97},
		{"RESERVED", 96},
		{"RESERVED", 95},
		{"RESERVED", 94},
		{"RESERVED", 93},
		{"RESERVED", 92},
		{"RESERVED", 91},
		{"RESERVED", 90},
		{"RESERVED", 89},
		{"RESERVED", 88},
		{"RESERVED", 87},
		{"RESERVED", 84},
		{"RESERVED", 83},
		{"RESERVED", 82},
		{"RESERVED", 81},
		{"RESERVED", 80},
		{"RESERVED", 69},
		{"RESERVED", 68},
		{"RESERVED", 67},
		{"RESERVED", 66},
		{"RESERVED", 65},
		{"RESERVED", 61},
		{"RESERVED", 59},
		{"RESERVED", 58},
		{"RESERVED", 57},
		{"RESERVED", 55},
		{"RESERVED", 51},
		{"RESERVED", 50},
		{"RESERVED", 49},
		{"RESERVED", 48},
		{"RESERVED", 43},
		{"RESERVED", 39},
		{"RESERVED", 38},
		{"RESERVED", 35},
		{"minimumTTL", 52},
		{"maximumTTL", 53},
		{"totalFlowCount", 3},
		{"tcpControlBits", 6},
		{"mplsLabelStackEntry9", 78},
		{"packetTotalCount", 86},
		{"packetDeltaCount", 139},
		{"sourceTransportPort", 7},
		{"mplsLabelStackEntry1", 70},
		{"sourceMacAddress", 56},
		{"sourceIPv4Address", 8},
		{"sourceIPv6Address", 27},
		{"mplsTopLabelIPv4Address", 47},
		{"mplsTopLabelIPv6Address", 140},
		{"minimumPacketLength", 25},
		{"maximumPacketLength", 26},
		{"mplsLabelStackEntry4", 73},
		{"sourceIPv4Prefix", 44},
		{"droppedOctetDeltaCount", 132},
		{"droppedOctetTotalCount", 134},
		{"droppedPacketDeltaCount", 133},
		{"droppedPacketTotalCount", 135},
		{"destinationTransportPort", 11},
		{"octetTotalCount", 85},
		{"octetDeltaCount", 138},
		{"mplsLabelStackEntry10", 79},
		{"destinationIPv4Address", 12},
		{"destinationIPv6Address", 28},
		{"outOctetDeltaCount", 23},
		{"outPacketDeltaCount", 24},
		{"mplsLabelStackEntry6", 75},
		{"destinationIPv4Prefix", 45},
		{"protocolIdentifier", 4},
		{"mplsLabelStackEntry3", 72},
		{"mplsTopLabelType", 46},
		{"exportedFlowCount", 42},
		{"exportedOctetCount", 40},
		{"exportedPacketCount", 41},
		{"mplsLabelStackEntry5", 74},
		{"classOfServiceIPv4", 5},
		{"exporterIPv4Address", 130},
		{"exporterIPv6Address", 131},
		{"mplsLabelStackEntry7", 76},
		{"inOctetDeltaCount", 1},
		{"inPacketDeltaCount", 2},
		{"ipVersion", 60},
		{"mplsLabelStackEntry8", 77},
		{"ipv6OptionHeaders", 64},
		{"classOfServiceIPv6", 137},
		{"sourceIPv4Mask", 9},
		{"sourceIPv6Mask", 29},
		{"ipNextHopIPv4Address", 15},
		{"ipNextHopIPv6Address", 62},
		{"flowActiveTimeOut", 36},
		{"identificationIPv4", 54},
		{"flowInactiveTimeout", 37},
		{"mplsLabelStackEntry2", 71},
		{"flowEndReason", 136},
		{"destinationIPv4Mask", 13},
		{"destinationIPv6Mask", 30},
		{"egressInterface", 14},
		{"ipNextHopAsNumber", 129},
		{"igmpType", 33},
		{"OutMulticastOctetCount", 20},
		{"OutMulticastPacketCount", 19},
		{"bgpNextHopIPv4Address", 18},
		{"bgpNextHopIPv6Address", 63},
		{"icmpTypeCode", 32},
		{"flowLabelIPv6", 31},
		{"ingressInterface", 10},
		{"flowEndTime", 21},
		{"bgpSourceAsNumber", 16},
		{"bgpNextHopAsNumber", 128},
		{"flowCreationTime", 22},
		{"bgpDestinationAsNumber", 17}
	};

	static const short lookup[] = {
		-75, -2, -70, -2, -65, -2, -63, -2,
		-239, -1, -141, -1, -61, -2, 67, -57,
		-2, -55, -2, 68, 69, -143, -52, -2,
		72, 73, 74, -145, -147, -153, 81, 82,
		-156, -158, 87, -163, 90, -192, 93, 94,
		95, 96, -1, 97, -1, 98, 99, 100,
		101, 102, 103, -49, -2, 104, -235, 107,
		-1, 108, 109, 110, 111, -1, 112, 113,
		-230, -212, -1, 118, 119, 120, 121, -24,
		-2, 122, -228, 125, -1, 126, 127, 128,
		129, -225, 132, 133, -10, -2, 134, -17,
		-2, -26, -2, 135, 136, 137, -35, -2,
		138, 139, -140, -65
	};

	if(len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH) {

		int key = hash(str, len);

		if(key <= MAX_HASH_VALUE && key >= 0) {
			int index = lookup[key];

			if(index >= 0) {
				const char *s = wordlist[index].name;

				if(*str == *s && !strcmp(str + 1, s + 1)) {
					return &wordlist[index];
				}

			} else if(index < -TOTAL_KEYWORDS) {
				int offset = -1 - TOTAL_KEYWORDS - index;
				const struct ipfix_midentifier *wordptr = &wordlist[TOTAL_KEYWORDS + lookup[offset]];
				const struct ipfix_midentifier *wordendptr = wordptr + -lookup[offset + 1];

				while(wordptr < wordendptr) {
					register const char *s = wordptr->name;

					if(*str == *s && !strcmp(str + 1, s + 1)) {
						return wordptr;
					}

					wordptr++;
				}
			}
		}
	}

	return 0;
}


/* lookup a certain ipfix ID into its name */
struct ipfix_identifier * ipfix_id_lookup(int n)
{
	if(n >= 0 && n < sizeof(IDTAB)/sizeof(*IDTAB)) {
		return &IDTAB[n];
	}

        return NULL;
}


/*
 lookup an ipfix name into its ID
 int because we need -1 for "not found"
 */
int ipfix_name_lookup(char *name)
{
        const struct ipfix_midentifier *tmp;

        if(!(tmp=in_word_set(name, strlen(name)))) {
                /* not found */
                return -1;
        }

        return (int)tmp->id;
}
