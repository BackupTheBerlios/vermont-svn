
typedef int BOOL;
#define TRUE 1
#define FALSE 0


enum type {	TYPE_TCP = 1,
 		TYPE_UDP,
		TYPE_ICMP,
		TYPE_ICMP6,
		TYPE_IP4,
		TYPE_IP6 };

typedef enum type e_type;
#define MAXFILENAME 20

enum e_format {HEX = 1, BYTE };
//#include "npag.h"



struct s_pattern {
	//int packet_size;
	int send_packets;
	double delay;
	int repeat;
	int payload_size;
	
	char payload_file[MAXFILENAME];
	int file_format;
	
	struct s_pattern *next;
};


struct s_traffic {
	int repeat;
	struct s_pattern *pattern;	
};

struct s_ip6conf {

	int traffic_class;
	int flowlabel;
	int nexthdr;
	int hoplimit;
	char interface[5];

	char src[16];      /* source address */
	char dst[16];      /* destination address */
};


struct s_ip4conf {

	int hl;
	int tos;			/* type of service */
	int id;			/* identification */
	int off;			/* fragment offset field */
	int ttl;			/* time to live */
	int *protocol;			/* protocol */
	char src[16];
	char dst[16];

	int *trans_proto_num;
};

struct s_tcpconf {

	//bool kernel_hdrincl;
	
	int mss;
	BOOL nodelay;

	int sport;
	int dport;
	int seq;
	int ack;
	int win;
	int urp;
	int off;

	BOOL FIN;
 	BOOL SYN;
 	BOOL RST;
 	BOOL PUSH;
 	BOOL ACK;
 	BOOL URG;
 	
 	int payload_size;
 	char pfile[MAXFILENAME];
 	
 //	int ip_src;
//	int ip_dst;
	
//	struct in6_addr dst_inaddr;
//	struct in6_addr src_inaddr;

	//struct addrinfo *ip_src;
	//struct addrinfo *ip_dst;

	struct addrinfo **ip_src_;
	struct addrinfo **ip_dst_;
	//int proto_num;
};

struct s_udpconf {
	int sport;
	int dport;
	int len;
	int payload_size;
	char pfile[MAXFILENAME];
	
	struct addrinfo **ip_src_;
	struct addrinfo **ip_dst_;
};
struct s_icmpconf {
	int type;
	int code;
	
	char gw_addr[16];
	short id;
	short seq;
	unsigned char pptr;
	/* only one of the options can be choosen */
	/*
	union {
		
		int gw_addr;
		unsigned char pptr;
		struct id_seq {
			short id;
			short seq;
		}id_seq;
	
		int mtu;
	}un;
 	*/
	
};

struct s_icmp6conf {
	unsigned char type;
	unsigned char code;
	
	unsigned int mtu;
	unsigned int pointer;
	
	unsigned short id;
	unsigned short seq;
	
	unsigned short mrd;
};



struct s_config {

	e_type transport;
	e_type network;

	int ipproto;
	BOOL kernel_packet;

//	struct addrinfo** ip_src;
//	struct addrinfo** ip_dst;

	void *net_proto;
	void *trans_proto;

	struct s_traffic traffic;
	struct s_config *next;
};



