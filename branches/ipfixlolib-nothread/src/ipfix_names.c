#include <string.h>
#include "ipfix_names.h"

static struct ipfix_identifier IDTAB[] = {
	{ "RESERVED", 0, 0 },
	{ "inoctetdeltacount", 1, 8 },
	{ "inpacketdeltacount", 2, 8 },
	{ "totalflowcount", 3, 8 },
	{ "protocolidentifier", 4, 1 },
	{ "classofserviceipv4", 5, 1 },
	{ "tcpcontrolbits", 6, 1 },
	{ "sourcetransportport", 7, 2 },
	{ "sourceipv4address", 8, 4 },
	{ "sourceipv4mask", 9, 1 },
	{ "ingressinterface", 10, 4 },
	{ "destinationtransportport", 11, 0 },
	{ "destinationipv4address", 12, 4 },
	{ "destinationipv4mask", 13, 1 },
	{ "egressinterface", 14, 4 },
	{ "ipnexthopipv4address", 15, 4 },
	{ "bgpsourceasnumber", 16, 2 },
	{ "bgpdestinationasnumber", 17, 2 },
	{ "bgpnexthopipv4address", 18, 4 },
	{ "outmulticastpacketcount", 19, 8 },
	{ "outmulticastoctetcount", 20, 8 },
	{ "flowendtime", 21, 4 },
	{ "flowcreationtime", 22, 4 },
	{ "outoctetdeltacount", 23, 8 },
	{ "outpacketdeltacount", 24, 8 },
	{ "minimumpacketlength", 25, 2 },
	{ "maximumpacketlength", 26, 2 },
	{ "sourceipv6address", 27, 8 },
	{ "destinationipv6address", 28, 8 },
	{ "sourceipv6mask", 29, 1 },
	{ "destinationipv6mask", 30, 0 },
	{ "flowlabelipv6", 31, 0 },
	{ "icmptypecode", 32, 2 },
	{ "igmptype", 33, 1 },
	{ "reserved", 34, 0 },
	{ "reserved", 35, 0 },
	{ "flowactivetimeout", 36, 2 },
	{ "flowinactivetimeout", 37, 2 },
	{ "reserved", 38, 0 },
	{ "reserved", 39, 0 },
	{ "exportedoctetcount", 40, 8 },
	{ "exportedpacketcount", 41, 8 },
	{ "exportedflowcount", 42, 8 },
	{ "reserved", 43, 0 },
	{ "sourceipv4prefix", 44, 4 },
	{ "destinationipv4prefix", 45, 0 },
	{ "mplstoplabeltype", 46, 1 },
	{ "mplstoplabelipv4address", 47, 4 },
	{ "reserved", 48, 0 },
	{ "reserved", 49, 0 },
	{ "reserved", 50, 0 },
	{ "reserved", 51, 0 },
	{ "minimumttl", 52, 1 },
	{ "maximumttl", 53, 1 },
	{ "identificationipv4", 54, 1 },
	{ "reserved", 55, 0 },
	{ "sourcemacaddress", 56, 1 },
	{ "reserved", 57, 0 },
	{ "reserved", 58, 0 },
	{ "reserved", 59, 0 },
	{ "ipversion", 60, 0 },
	{ "reserved", 61, 0 },
	{ "ipnexthopipv6address", 62, 8 },
	{ "bgpnexthopipv6address", 63, 8 },
	{ "ipv6optionheaders", 64, 4 },
	{ "reserved", 65, 0 },
	{ "reserved", 66, 0 },
	{ "reserved", 67, 0 },
	{ "reserved", 68, 0 },
	{ "reserved", 69, 0 },
	{ "mplslabelstackentry1", 70, 4 },
	{ "mplslabelstackentry2", 71, 4 },
	{ "mplslabelstackentry3", 72, 4 },
	{ "mplslabelstackentry4", 73, 4 },
	{ "mplslabelstackentry5", 74, 4 },
	{ "mplslabelstackentry6", 75, 4 },
	{ "mplslabelstackentry7", 76, 4 },
	{ "mplslabelstackentry8", 77, 4 },
	{ "mplslabelstackentry9", 78, 4 },
	{ "mplslabelstackentry10", 79, 4 },
	{ "reserved", 80, 0 },
	{ "reserved", 81, 0 },
	{ "reserved", 82, 0 },
	{ "reserved", 83, 0 },
	{ "reserved", 84, 0 },
	{ "octettotalcount", 85, 8 },
	{ "packettotalcount", 86, 8 },
	{ "reserved", 87, 0 },
	{ "reserved", 88, 0 },
	{ "reserved", 89, 0 },
	{ "reserved", 90, 0 },
	{ "reserved", 91, 0 },
	{ "reserved", 92, 0 },
	{ "reserved", 93, 0 },
	{ "reserved", 94, 0 },
	{ "reserved", 95, 0 },
	{ "reserved", 96, 0 },
	{ "reserved", 97, 0 },
	{ "reserved", 98, 0 },
	{ "reserved", 99, 0 },
	{ "reserved", 100, 0 },
	{ "reserved", 101, 0 },
	{ "reserved", 102, 0 },
	{ "reserved", 103, 0 },
	{ "reserved", 104, 0 },
	{ "reserved", 105, 0 },
	{ "reserved", 106, 0 },
	{ "reserved", 107, 0 },
	{ "reserved", 108, 0 },
	{ "reserved", 109, 0 },
	{ "reserved", 110, 0 },
	{ "reserved", 111, 0 },
	{ "reserved", 112, 0 },
	{ "reserved", 113, 0 },
	{ "reserved", 114, 0 },
	{ "reserved", 115, 0 },
	{ "reserved", 116, 0 },
	{ "reserved", 117, 0 },
	{ "reserved", 118, 0 },
	{ "reserved", 119, 0 },
	{ "reserved", 120, 0 },
	{ "reserved", 121, 0 },
	{ "reserved", 122, 0 },
	{ "reserved", 123, 0 },
	{ "reserved", 124, 0 },
	{ "reserved", 125, 0 },
	{ "reserved", 126, 0 },
	{ "reserved", 127, 0 },
	{ "bgpnexthopasnumber", 128, 2 },
	{ "ipnexthopasnumber", 129, 2 },
	{ "exporteripv4address", 130, 4 },
	{ "exporteripv6address", 131, 8 },
	{ "droppedoctetdeltacount", 132, 8 },
	{ "droppedpacketdeltacount", 133, 8 },
	{ "droppedoctettotalcount", 134, 8 },
	{ "droppedpackettotalcount", 135, 8 },
	{ "flowendreason", 136, 1 },
	{ "classofserviceipv6", 137, 1 },
	{ "octetdeltacount", 138, 8 },
	{ "packetdeltacount", 139, 8 },
	{ "mplstoplabelipv6address", 140, 8 },
};


/*
 ANSI-C code produced by gperf version 2.7
 Command-line: gperf -D -C -t -L ANSI-C
 */
struct ipfix_midentifier {
	char *name;
	uint16_t id;
};

#define TOTAL_KEYWORDS 141
#define MIN_WORD_LENGTH 8
#define MAX_WORD_LENGTH 24
#define MIN_HASH_VALUE 10
#define MAX_HASH_VALUE 136
/* maximum key range = 127, duplicates = 79 */

static unsigned int hash (register const char *str, register unsigned int len)
{
	static const unsigned char asso_values[] = {
		137, 137, 137, 137, 137, 137, 137, 137, 137, 137,
		137, 137, 137, 137, 137, 137, 137, 137, 137, 137,
		137, 137, 137, 137, 137, 137, 137, 137, 137, 137,
		137, 137, 137, 137, 137, 137, 137, 137, 137, 137,
		137, 137, 137, 137, 137, 137, 137, 137,   5,   5,
		10,  20,  15,  25,   0,  30,  35,  40, 137, 137,
		137, 137, 137, 137, 137, 137, 137, 137, 137, 137,
		137, 137, 137, 137, 137, 137, 137, 137, 137, 137,
		137, 137, 137, 137, 137, 137, 137, 137, 137, 137,
		137, 137, 137, 137, 137, 137, 137, 137,  60,  10,
		50,   0,  52, 137,  40,  40, 137,  25,   0,   0,
		15,  61,  15, 137,   0,  15,  52, 137, 137, 137,
		20, 137, 137, 137, 137, 137, 137, 137, 137, 137,
		137, 137, 137, 137, 137, 137, 137, 137, 137, 137,
		137, 137, 137, 137, 137, 137, 137, 137, 137, 137,
		137, 137, 137, 137, 137, 137, 137, 137, 137, 137,
		137, 137, 137, 137, 137, 137, 137, 137, 137, 137,
		137, 137, 137, 137, 137, 137, 137, 137, 137, 137,
		137, 137, 137, 137, 137, 137, 137, 137, 137, 137,
		137, 137, 137, 137, 137, 137, 137, 137, 137, 137,
		137, 137, 137, 137, 137, 137, 137, 137, 137, 137,
		137, 137, 137, 137, 137, 137, 137, 137, 137, 137,
		137, 137, 137, 137, 137, 137, 137, 137, 137, 137,
		137, 137, 137, 137, 137, 137, 137, 137, 137, 137,
		137, 137, 137, 137, 137, 137, 137, 137, 137, 137,
		137, 137, 137, 137, 137, 137
	};

	return len + asso_values[(unsigned char)str[len - 1]] + asso_values[(unsigned char)str[0]];
}

const struct ipfix_midentifier * in_word_set (register const char *str, register unsigned int len)
{
	static const struct ipfix_midentifier wordlist[] = {
		{"minimumttl", 52},
		{"maximumttl", 53},
		{"egressinterface", 14},
		{"mplstoplabeltype", 46},
		{"mplslabelstackentry6", 75},
		{"mplslabelstackentry1", 70},
		{"mplslabelstackentry10", 79},
		{"classofserviceipv6", 137},
		{"mplslabelstackentry2", 71},
		{"protocolidentifier", 4},
		{"exporteripv4address", 130},
		{"exporteripv6address", 131},
		{"mplslabelstackentry4", 73},
		{"mplstoplabelipv4address", 47},
		{"mplstoplabelipv6address", 140},
		{"mplslabelstackentry3", 72},
		{"classofserviceipv4", 5},
		{"mplslabelstackentry5", 74},
		{"sourcemacaddress", 56},
		{"sourceipv4address", 8},
		{"sourceipv6address", 27},
		{"igmptype", 33},
		{"mplslabelstackentry7", 76},
		{"sourceipv4prefix", 44},
		{"icmptypecode", 32},
		{"sourceipv4mask", 9},
		{"sourceipv6mask", 29},
		{"mplslabelstackentry8", 77},
		{"ingressinterface", 10},
		{"ipnexthopasnumber", 129},
		{"reserved", 0},
		{"reserved", 127},
		{"reserved", 126},
		{"reserved", 125},
		{"reserved", 124},
		{"reserved", 123},
		{"reserved", 122},
		{"reserved", 121},
		{"reserved", 120},
		{"reserved", 119},
		{"reserved", 118},
		{"reserved", 117},
		{"reserved", 116},
		{"reserved", 115},
		{"reserved", 114},
		{"reserved", 113},
		{"reserved", 112},
		{"reserved", 111},
		{"reserved", 110},
		{"reserved", 109},
		{"reserved", 108},
		{"reserved", 107},
		{"reserved", 106},
		{"reserved", 105},
		{"reserved", 104},
		{"reserved", 103},
		{"reserved", 102},
		{"reserved", 101},
		{"reserved", 100},
		{"reserved", 99},
		{"reserved", 98},
		{"reserved", 97},
		{"reserved", 96},
		{"reserved", 95},
		{"reserved", 94},
		{"reserved", 93},
		{"reserved", 92},
		{"reserved", 91},
		{"reserved", 90},
		{"reserved", 89},
		{"reserved", 88},
		{"reserved", 87},
		{"reserved", 84},
		{"reserved", 83},
		{"reserved", 82},
		{"reserved", 81},
		{"reserved", 80},
		{"reserved", 69},
		{"reserved", 68},
		{"reserved", 67},
		{"reserved", 66},
		{"reserved", 65},
		{"reserved", 61},
		{"reserved", 59},
		{"reserved", 58},
		{"reserved", 57},
		{"reserved", 55},
		{"reserved", 51},
		{"reserved", 50},
		{"reserved", 49},
		{"reserved", 48},
		{"reserved", 43},
		{"reserved", 39},
		{"reserved", 38},
		{"reserved", 35},
		{"reserved", 34},
		{"minimumpacketlength", 25},
		{"maximumpacketlength", 26},
		{"mplslabelstackentry9", 78},
		{"flowendtime", 21},
		{"ipversion", 60},
		{"flowlabelipv6", 31},
		{"flowcreationtime", 22},
		{"exportedflowcount", 42},
		{"exportedoctetcount", 40},
		{"exportedpacketcount", 41},
		{"ipv6optionheaders", 64},
		{"identificationipv4", 54},
		{"ipnexthopipv4address", 15},
		{"ipnexthopipv6address", 62},
		{"bgpsourceasnumber", 16},
		{"bgpnexthopasnumber", 128},
		{"flowendreason", 136},
		{"tcpcontrolbits", 6},
		{"bgpdestinationasnumber", 17},
		{"packettotalcount", 86},
		{"packetdeltacount", 139},
		{"sourcetransportport", 7},
		{"destinationipv4address", 12},
		{"destinationipv6address", 28},
		{"destinationipv4prefix", 45},
		{"destinationipv4mask", 13},
		{"destinationipv6mask", 30},
		{"bgpnexthopipv4address", 18},
		{"bgpnexthopipv6address", 63},
		{"inoctetdeltacount", 1},
		{"inpacketdeltacount", 2},
		{"totalflowcount", 3},
		{"flowactivetimeout", 36},
		{"flowinactivetimeout", 37},
		{"droppedoctetdeltacount", 132},
		{"droppedoctettotalcount", 134},
		{"droppedpacketdeltacount", 133},
		{"droppedpackettotalcount", 135},
		{"destinationtransportport", 11},
		{"octettotalcount", 85},
		{"octetdeltacount", 138},
		{"outoctetdeltacount", 23},
		{"outpacketdeltacount", 24},
		{"outmulticastoctetcount", 20},
		{"outmulticastpacketcount", 19}
	};

	static const short lookup[] = {
		-1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
		-1,   -1, -153, -141,   -2,   -1,   -1,    2,
		3,   -1,   -1,   -1,    4,   -1,   -1,   -1,
		-1,    5,    6,   -1,    7,   -1,    8,   -1,
		-1,    9, -178,   12, -131,   -2, -183,   -1,
		15, -128,   -2,   16,   -1,   17,   18, -245,
		21,   -1,   22,   23,   24,   -1, -243,   27,
		28,   29, -208, -203,   98,  -45,   -2,   99,
		100,  101, -111,  -66,  102,  103,  104,  105,
		106,  107,   -1, -234,   -1,  110,  111,   -1,
		112,  113,  114, -226,  -26,   -2,  117, -230,
		-23,   -2,   -1,  120,  -33,   -2, -241,   -1,
		-239,  -18,   -2,  -20,   -2, -116,   -2, -122,
		-2,   -1,   -1,   -1,   -1,  125,  126,   -1,
		-1,   -1,   -1,   -1,   -1,   -1,  127,  -11,
		-2,  128,   -1,  129, -261, -275,  134,   -1,
		-271,   -6,   -2,  137,  138,   -9,   -2,  139,
		140
	};

	if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH) {
		register int key = hash (str, len);

		if (key <= MAX_HASH_VALUE && key >= 0) {
			register int index = lookup[key];

			if (index >= 0) {
				register const char *s = wordlist[index].name;

				if (*str == *s && !strcmp (str + 1, s + 1))
					return &wordlist[index];

			} else if (index < -TOTAL_KEYWORDS) {
				register int offset = - 1 - TOTAL_KEYWORDS - index;
				register const struct ipfix_midentifier *wordptr = &wordlist[TOTAL_KEYWORDS + lookup[offset]];
				register const struct ipfix_midentifier *wordendptr = wordptr + -lookup[offset + 1];

				while (wordptr < wordendptr) {
					register const char *s = wordptr->name;

					if (*str == *s && !strcmp (str + 1, s + 1))
						return wordptr;
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
