#include <stdio.h>
#include <string.h>
#include <stdlib.h> //atof
#include <netdb.h>
#include "memory.h"

#include "config.h"
#include "automata.h"
#include "states.h"

//TODO if you change conf->ipproto you may not need this anymore ?!
#include <netinet/in.h> //IPPROTO_XXX


extern int fp_line; /* line of FILE pointer */
extern int word_line;	/* line of word that was read last */

extern struct addrinfo **tmpip_src;
extern struct addrinfo **tmpip_dst;
extern int *tmpipproto;


/****************************************************************************
 * First state of the automata. A new stream specification should start e.g.
 * 'sender' is the next word that is expected.
 ****************************************************************************/
void begin(char *word, struct s_config *conf, struct s_automata *automata) {
	if( strcmp(word, "sender") == 0 ) {

		//TODO initialize conf
		automata->state = STREAM;
	}

	/* Error: new stream specification should begin with word 'sender' */
	else {
		fprintf(stderr, "ERROR line %i: got '%s' but 'sender' expected\n", word_line, word);
		exit(0);
	}
}

void init_stream(struct s_config *conf) {
	/* empty */
}
void check_stream(struct s_config *conf) {
	/* empty */
}
void read_stream(char *word, char *val, struct s_config *conf, struct s_automata *automata) {
/*
	struct s_node *node;
	node = &head;
	node = node->next;
	
	
	while(node != NULL) {
		if(strcmp(word, node->trigger) == 0) {
			automata->state = node->state;
			node->init(conf);
			break;
		}
		node = node->next;	
	}*/
	
	//if(node == NULL) {
		/*
		if(strcmp(word, "traffic") == 0) {
			if(automata->traffic_defined) {
				fprintf(stderr, "ERROR line %i: traffic was previously defined\n", word_line);
			}
			automata->state = TRAFFIC;
	     }*/
		
		if(strcmp(word, "packet") == 0) {
			if(strcmp(val, "kernel") == 0) {
				conf->kernel_packet = TRUE;
			}
			else if(strcmp(val, "raw") == 0) {
				conf->kernel_packet = FALSE;
			}
			else {
				fprintf(stderr, "ERROR line %i: unknown parameter '%s'. Use "\
					"'kernel' or 'raw' instead!\n", fp_line, val);
				exit(0);
			}
		}
		
		 else if( strcmp(word, "}" ) == 0) {
			if(!automata->network_defined) {
				fprintf(stderr, "ERROR line %i: ip or ip6 expected\n", word_line);
				exit(0);
			}
			if(!automata->transport_defined) {
				fprintf(stderr, "WARNING line %i: no transport protocol defined\n", word_line);
			}
			if(!automata->traffic_defined) {
				fprintf(stderr, "ERROR line %i: no traffic parameters defined\n", word_line);
				exit(0);
			}
			automata->reset = TRUE;
			automata->state = BEGIN;
		}

		else {
			fprintf(stderr, "ERROR line %i: unknown protocol - %s\n", word_line, word);
			exit(0);
		}
	//}


}


/* -------------------------------------------------------------------------------------
 				IP protocol specification
----------------------------------------------------------------------------------------*/

//void init_ip(struct s_ip4conf* ip) {
void init_ip(struct s_config* conf) {
	conf->network = TYPE_IP4;
	conf->net_proto = (struct s_ip4conf*)malloc(sizeof(struct s_ip4conf));
	struct s_ip4conf *ip = conf->net_proto;
	
	//TODO this only makes sence if no options can be specified!
	ip->hl = 5;
	ip->off = 0;
	ip->protocol = tmpipproto;
	ip->ttl = 120;
	ip->id = 0;
	ip->tos = 0;
		
	//strcpy(ip->src,"127.0.0.1");
}
void check_ip(struct s_config* conf) {
	struct s_ip4conf *ip = conf->net_proto;
	
	if(strcmp(ip->dst,"") == 0) {
		fprintf(stderr, "ERROR line %i: no IP destination address defined\n", word_line);
		exit(0);
	}
	if(strcmp(ip->src,"") == 0) {
		fprintf(stderr, "ERROR line %i: no IP source address defined\n", word_line);
		exit(0);
	}
	
}
void read_ip(char *word, char *val, struct s_config *conf, struct s_automata *automata) {

	struct s_ip4conf *ip = conf->net_proto;

	if(strcmp(word, "src") == 0) {
		int ret;
		strcpy(ip->src, val);

		//conf->ip_src = (struct addrinfo**)malloc(sizeof(struct addrinfo*));
		//ret = getaddrinfo(val, NULL, NULL, &(*conf->ip_src));
		ret = getaddrinfo(val, NULL, NULL, &(*tmpip_src));
		
		if(ret != 0) {
			perror("getaddrinfo src");
			exit(0);
		}
	}
	else if(strcmp(word, "dst") == 0) {
		int ret;
		strcpy(ip->dst, val);

		//ret = getaddrinfo(val, NULL, NULL, &(*conf->ip_dst));
		ret = getaddrinfo(val, NULL, NULL, &(*tmpip_dst));
		if(ret != 0) {
			perror("getaddrinfo src");
			exit(0);
		}
	}
	else if(strcmp(word, "ttl") == 0) {
		ip->ttl = atoi(val);
	}
	else if(strcmp(word, "id") == 0) {
		ip->id = atoi(val);
		if(conf->kernel_packet) 
			fprintf(stdout, "WARNING line %i: with kernel packets the IP id option is omitted\n"\
							"\t(this option is specified by the kernel\n", word_line);			
	}

	else if(strcmp(word, "tos") == 0) {
		ip->tos = atoi(val);
	}

	else if(strcmp(word, "}") == 0) {
		check_ip(conf);
		automata->network_defined = TRUE;
		automata->state = STREAM;
	}

	else {
		fprintf(stderr, "ERROR line %i: unknown IP parameter - %s\n", word_line, word);
		exit(0);
	}
}

/* -------------------------------------------------------------------------------------
 				IP version 6 protocol specification
----------------------------------------------------------------------------------------*/

void init_ip6(struct s_config* conf) {
	conf->network = TYPE_IP6;
	conf->net_proto = (struct s_ip6conf*)malloc(sizeof(struct s_ip6conf));
	struct s_ip6conf* ip6 = conf->net_proto;
	ip6->traffic_class = 0;
	ip6->flowlabel = 0;
	ip6->nexthdr = *tmpipproto;
	ip6->hoplimit = 64;
}
void check_ip6(struct s_config* conf) {
	struct s_ip6conf* ip6 = conf->net_proto;
	
	if(strcmp(ip6->dst,"") == 0) {
		fprintf(stderr, "ERROR line %i: no IP6 destination address defined\n", word_line);
		exit(0);
	}
}
void read_ip6(char *word, char *val, struct s_config *conf, struct s_automata *automata) {
	
	struct s_ip6conf *ip6 = conf->net_proto;

	if(strcmp(word, "src") == 0) {
		int ret;
		strcpy(ip6->src, val);

		//conf->ip_src = (struct addrinfo**)malloc(sizeof(struct addrinfo*));
		//ret = getaddrinfo(val, NULL, NULL, &(*conf->ip_src));
		ret = getaddrinfo(val, NULL, NULL, &(*tmpip_src));
		
		if(ret != 0) {
			perror("getaddrinfo src");
			exit(0);
		}
	}
	else if(strcmp(word, "dst") == 0) {
		int ret;
		strcpy(ip6->dst, val);

		//ret = getaddrinfo(val, NULL, NULL, &(*conf->ip_dst));
		ret = getaddrinfo(val, NULL, NULL, &(*tmpip_dst));
		if(ret != 0) {
			perror("getaddrinfo src");
			exit(0);
		}
	}
	else if(strcmp(word, "hoplimit") == 0) {
		ip6->hoplimit = atoi(val);
	}
	else if(strcmp(word, "flowlabel") == 0) {
		ip6->flowlabel = atoi(val);
	}
	else if(strcmp(word, "tc") == 0) {
		ip6->traffic_class = atoi(val);
	}
	else if(strcmp(word, "interface") == 0) {
		strcpy(ip6->interface, val);;
	}
	
	else if(strcmp(word, "}") == 0) {
		check_ip6(conf);
		automata->network_defined = TRUE;
		automata->state = STREAM;
	}

	else {
		fprintf(stderr, "ERROR line %i: unknown IPv6 parameter - %s\n", word_line, word);
		exit(0);
	}
}


/* -------------------------------------------------------------------------------------
 				ICMP protocol specification
----------------------------------------------------------------------------------------*/

void init_icmp(struct s_config* conf) {
	conf->transport = TYPE_ICMP;
	conf->ipproto = IPPROTO_ICMP; //TODO conf->transport is actually the same...
//	conf->kernel_packet = FALSE; 
	conf->trans_proto = (struct s_icmpconf*)malloc(sizeof(struct s_icmpconf));
	struct s_icmpconf* icmp = conf->trans_proto;
	icmp->type = 3;
	icmp->code = 0;
	
	icmp->id = 0;
	icmp->seq = 0;
	//icmp->mtu = -1;
	icmp->pptr = 0;
	//icmp->gw_addr = "";
	*tmpipproto = IPPROTO_ICMP;
}
void check_icmp(struct s_config* conf) {
}
void read_icmp(char *word, char *val, struct s_config *conf, struct s_automata *automata) {
	struct s_icmpconf *icmp = conf->trans_proto;
	
	if(strcmp(word, "type") == 0) {
		icmp->type = atoi(val);
	}
	else if(strcmp(word, "code") == 0) {
		icmp->code = atoi(val);
	}
	else if(strcmp(word, "id") == 0) {
		icmp->id = atoi(val);
	}
	else if(strcmp(word, "seq") == 0) {
		icmp->seq = atoi(val);
	}
	else if(strcmp(word, "pointer") == 0) {
		icmp->pptr = (unsigned char)atoi(val);
	}
	else if(strcmp(word, "gw_addr") == 0) {
		strncpy(icmp->gw_addr, val, 16);
	}
	else if(strcmp(word, "}") == 0) {
		automata->transport_defined = TRUE;
		automata->state = STREAM;
	}
	else {
		fprintf(stderr, "ERROR line %i: unknown icmp parameter - %s\n", word_line, word);
		exit(0);
	}
	
}


/* -------------------------------------------------------------------------------------
 				ICMP VERSION 6 protocol specification
----------------------------------------------------------------------------------------*/
void init_icmp6(struct s_config *conf) {
	conf->transport = TYPE_ICMP6;
//	conf->ipproto = IPPROTO_ICMP6; //TODO conf->transport is actually the same...
	conf->trans_proto = (struct s_icmp6conf*)malloc(sizeof(struct s_icmp6conf));
	struct s_icmp6conf* icmp6 = conf->trans_proto;
	conf->ipproto = IPPROTO_ICMPV6;
	icmp6->id = 0;
	icmp6->seq = 0;
	icmp6->pointer = 0;
	icmp6->mtu = 0;
	icmp6->code = 0;
	
}
void check_icmp6(struct s_config *conf) {
}
void read_icmp6(char *word, char *val, struct s_config *conf, struct s_automata *automata) {
	struct s_icmp6conf *icmp6 = conf->trans_proto;
	
	if(strcmp(word, "type") == 0) {
		icmp6->type = atoi(val);
	}
	else if(strcmp(word, "code") == 0) {
		icmp6->code = atoi(val);
	}
	else if(strcmp(word, "mtu") == 0) {
		icmp6->mtu = atoi(val);
	}
	else if(strcmp(word, "pointer") == 0) {
		icmp6->pointer = atoi(val);
	}
	else if(strcmp(word, "id") == 0) {
		icmp6->id = atoi(val);
	}
	else if(strcmp(word, "seq") == 0) {
		icmp6->seq = atoi(val);
	}
	else if(strcmp(word, "mrd") == 0) {
		icmp6->mrd = atoi(val);
	}
	else if(strcmp(word, "}") == 0) {
		automata->transport_defined = TRUE;
		automata->state = STREAM;
	}
	else {
		fprintf(stderr, "ERROR line %i: unknown icmp6 parameter - %s\n", word_line, word);
		exit(0);
	}
}




/* -------------------------------------------------------------------------------------
 				TCP protocol specification
----------------------------------------------------------------------------------------*/

void init_tcp(struct s_config* conf) {
	
	conf->transport = TYPE_TCP;
	conf->ipproto = IPPROTO_TCP; //TODO conf->transport is actually the same...
//	conf->kernel_packet = TRUE; 
	conf->trans_proto = (struct s_tcpconf*)malloc(sizeof(struct s_tcpconf));
	struct s_tcpconf* tcp = conf->trans_proto;
	tcp->mss = 1500;
	tcp->seq = 0;
	tcp->win = 512;
	tcp->urp = 0;
	tcp->off = 5; //TODO this only makes sence if no options can be specified!
	tcp->nodelay = FALSE;
	//tcp->payload_size = 0;
	*tmpipproto = IPPROTO_TCP;
}
void check_tcp(struct s_config* conf) {
	struct s_tcpconf* tcp = conf->trans_proto;
	
	if(tcp->dport == 0) {
		fprintf(stderr, "ERROR line %i: no TCP destination port defined\n", word_line);
		exit(0);
	}
}

void read_tcp(char *word, char *val, struct s_config *conf, struct s_automata *automata) {

	struct s_tcpconf *tcp = conf->trans_proto;


	 if(strcmp(word, "mss") == 0) {
		tcp->mss = atoi(val);
	}
	else if(strcmp(word, "payloadsize") == 0) {
		tcp->payload_size = atoi(val);
	}
	else if(strcmp(word, "win") == 0) {
		tcp->win = atoi(val);
	}
	else if(strcmp(word, "sport") == 0) {
		tcp->sport = atoi(val);
	}
	else if(strcmp(word, "dport") == 0) {
		tcp->dport = atoi(val);
	}
//	else if(strcmp(word, "payloadfile") == 0) {
//		strncpy(tcp->pfile, val, 20);
//	}
	else if(strcmp(word, "seq") == 0) {
		tcp->seq = atoi(val);
		if(conf->kernel_packet)
			fprintf(stdout, "WARNING line %i: with kernel packets the sequence number option is omitted\n"\
						"\t(this option is specified by the kernel)\n", word_line);
	}
	else if(strcmp(word, "ack") == 0) {
		tcp->ACK = atoi(val);
		if(conf->kernel_packet)
			fprintf(stdout, "WARNING line %i: with kernel packets the ACK flag option is omitted\n"\
						"\t(this option is specified by the kernel)\n", word_line);
	}
	else if(strcmp(word, "acknum") == 0) {
		tcp->ack = atoi(val);
		if(conf->kernel_packet)
			fprintf(stdout, "WARNING line %i: with kernel packets the ACK number option is omitted\n"\
						"\t(this option is specified by the kernel)\n", word_line);
	}
	else if(strcmp(word, "syn") == 0) {
		tcp->SYN = atoi(val);
		if(conf->kernel_packet)
			fprintf(stdout, "WARNING line %i: with kernel packets the SYN flag option is omitted\n"\
						"\t(this option is specified by the kernel)\n", word_line);
	}
	else if(strcmp(word, "push") == 0) {
		tcp->PUSH = atoi(val);
		if(conf->kernel_packet)
			fprintf(stdout, "WARNING line %i: with kernel packets the PUSH flag option is omitted\n"\
						"\t(this option is specified by the kernel)\n", word_line);
	}
	else if(strcmp(word, "fin") == 0) {
		tcp->FIN = atoi(val);
		if(conf->kernel_packet)
			fprintf(stdout, "WARNING line %i: with kernel packets the FIN flag option is omitted\n"\
						"\t(this option is specified by the kernel)\n", word_line);
	}
	else if(strcmp(word, "urg") == 0) {
		tcp->URG = atoi(val);
		if(conf->kernel_packet)
			fprintf(stdout, "WARNING line %i: with kernel packets the URG flag option is omitted\n"\
						"\t(this option is specified by the kernel)\n", word_line);
	}
	else if(strcmp(word, "rst") == 0) {
		tcp->RST = atoi(val);
		if(conf->kernel_packet)
			fprintf(stdout, "WARNING line %i: with kernel packets the RST flag option is omitted\n"\
						"\t(this option is specified by the kernel)\n", word_line);
	}
	else if(strcmp(word, "nodelay") == 0) {
		tcp->nodelay = atoi(val);
	}


	else if(strcmp(word, "}") == 0) {
		check_tcp(conf);
		//tcp->ip_src_ = conf->ip_src;
		//tcp->ip_dst_ = conf->ip_dst;
		tcp->ip_src_ = tmpip_src;
		tcp->ip_dst_ = tmpip_dst;
		
		automata->transport_defined = TRUE;
		automata->state = STREAM;
	}

	else {
		fprintf(stderr, "ERROR line %i: unknown parameter - %s\n", word_line, word);
		exit(0);
	}
	
}

/* -------------------------------------------------------------------------------------
 				UDP protocol specification
----------------------------------------------------------------------------------------*/

void init_udp(struct s_config* conf) {
	conf->transport = TYPE_UDP;
	conf->ipproto = IPPROTO_UDP; //TODO conf->transport is actually the same...
	conf->kernel_packet = TRUE;
	conf->trans_proto = (struct s_udpconf*)malloc(sizeof(struct s_udpconf));
	struct s_udpconf* udp = conf->trans_proto;
	udp->sport = 5000;
	udp->len = 3;
	*tmpipproto = IPPROTO_UDP;
}
void check_udp(struct s_config* conf) {
	struct s_udpconf* udp = conf->trans_proto;
	if(udp->dport == 0) {
		fprintf(stderr, "ERROR line %i: no UDP destination port defined\n", word_line);
		exit(0);
	}
}
void read_udp(char *word, char *val, struct s_config *conf, struct s_automata *automata) {
	struct s_udpconf *udp = conf->trans_proto;
	
	if(strcmp(word, "packet") == 0) {
		if(strcmp(val, "kernel") == 0) {
			conf->kernel_packet = TRUE;
		//	tcp->kernel_hdrincl = TRUE;
		}
		else if(strcmp(val, "raw") == 0) {
			conf->kernel_packet = FALSE;
		//	tcp->kernel_hdrincl = FALSE;
		}
		else {
			fprintf(stderr, "ERROR line %i: unknown parameter '%s'. Use "\
					"'kernel' or 'raw' instead!\n", fp_line, val);
			exit(0);
		}
	}
	
	else if(strcmp(word, "sport") == 0) {
		udp->sport = atoi(val);
	}

	else if(strcmp(word, "dport") == 0) {
		udp->dport = atoi(val);
	}
	
	else if(strcmp(word, "payloadsize") == 0) {
		udp->payload_size = atoi(val);
	}
	else if(strcmp(word, "payloadfile") == 0) {
		strncpy(udp->pfile, val, 20);
	}
	
	else if(strcmp(word, "}") == 0) {
		check_udp(conf);
		udp->ip_src_ = tmpip_src;
		udp->ip_dst_ = tmpip_dst;
		automata->transport_defined = TRUE;
		automata->state = STREAM;
	}

	else {
		fprintf(stderr, "ERROR line %i: unknown parameter - %s\n", word_line, word);
		exit(0);
	}
}

/* -------------------------------------------------------------------------------------
 				Traffic specification
----------------------------------------------------------------------------------------*/
void init_traffic(struct s_config *conf) {
	/* empty */
}
void check_traffic(struct s_config *conf) {
	/* empty */
}
void read_traffic(char *word, char *val, struct s_config *conf, struct s_automata *automata) {

//	struct s_pattern* pattern;
	
	/*
	if(strcmp(word, "pattern") == 0) {
		pattern = conf->traffic.pattern;

		if(conf->traffic.pattern == NULL) {
			conf->traffic.pattern = (struct s_pattern*)malloc(sizeof(struct s_pattern));
			conf->traffic.pattern->next = NULL;
		}
		else {
			// goto last element of linked list 
			while(pattern->next != NULL)
				pattern = pattern->next;

			pattern->next = (struct s_pattern*)malloc(sizeof(struct s_pattern));
			pattern = pattern->next;
			pattern->next = NULL;
		}
		automata->state = PATTERN;
	}
	*/
	if(strcmp(word, "repeat") == 0) {	
		conf->traffic.repeat = atoi(val);
	}
	
	else if(strcmp(word, "}") == 0) {
		automata->traffic_defined = TRUE;
		automata->state = STREAM;
	}

	else {
		fprintf(stderr, "ERROR line %i: unknown traffic parameter - %s\n", word_line, word);
		exit(0);
	}

}

void init_pattern(struct s_config *conf) {
		
		if(conf->traffic.pattern == NULL) {
			conf->traffic.pattern = (struct s_pattern*)malloc(sizeof(struct s_pattern));
			conf->traffic.pattern->next = NULL;
		}
		else {
			struct s_pattern *pattern = conf->traffic.pattern; 
			// goto last element of linked list 
			while(pattern->next != NULL)
				pattern = pattern->next;

			pattern->next = (struct s_pattern*)malloc(sizeof(struct s_pattern));
			pattern = pattern->next;
			pattern->next = NULL;
		}	
}

void check_pattern(struct s_config *conf) {
	/* empty */
}
void read_pattern(char *word, char *val, struct s_config *conf, struct s_automata *automata) {
	struct s_pattern *pattern = conf->traffic.pattern;

	/* goto last element of linked list */
	while(pattern->next != NULL)
		pattern = pattern->next;

	if(strcmp(word, "sendpackets") == 0) {
		pattern->send_packets = atoi(val);
	}
	else if(strcmp(word, "delay") == 0) {
		pattern->delay = atof(val);
	}
	else if(strcmp(word, "repeat") == 0) {
		pattern->repeat = atoi(val);
	}
	else if(strcmp(word, "size") == 0) {
		pattern->payload_size = atof(val);
	}
	else if(strcmp(word, "file_byte") == 0) {
		strncpy(pattern->payload_file, val, 20);
		pattern->file_format = BYTE;	
	}
	else if(strcmp(word, "file_hex") == 0) {
		strncpy(pattern->payload_file, val, 20);
		pattern->file_format = HEX;	
	}
	else if(strcmp(word, "}") == 0) {
		automata->state = TRAFFIC;
	}
	/*
	else if(strcmp(word, "packetsize") == 0) {
		pattern->packet_size = atoi(val);
	}*/
	else {
		fprintf(stderr, "ERROR line %i: unknown pattern parameter - %s\n", word_line, word);
		exit(0);
	}

}
