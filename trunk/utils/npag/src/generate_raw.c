#define _BSD_SOURCE
//#define __USE_BSD


#include <sys/socket.h>
#include <stdio.h>
#include <netinet/tcp.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <netinet/ip_icmp.h>
#include <netinet/ip6.h>
#include <netinet/icmp6.h>
#include <arpa/inet.h> //inet_aton
#include <string.h> //memset

#include <stdlib.h> //rand
#include <net/if.h> //if_nametoindex
#include "memory.h"


#include "packet.h"
#include "config.h"


void check(char *msg, int c);
u_int16_t tcp_checksum();
int checksum ();
void set_payload();

char* get_p(struct s_buffer *buffer, int size);
typedef int sock_descriptor_t;

extern int verbose;
//int set_payload(char *buf, int size, char *filename, int pos);
/****************************************************************
 * Interface functions for struct s_generator. 					*
 * The s_generator structure has three pointers to functions:	*
 * 1) init_socket												*
 * 2) set_nethdr												*
 * 3) set_transhdr												*
 * 																*
 ****************************************************************/

/* Interface for init_socket */
void init_rawsocket(sock_descriptor_t *fd, struct s_config *conf) {
	int ret;
	int one = 1;
	struct sockaddr_in dst_addr;
	struct s_ip4conf *ipconf;
	
	ipconf = (struct s_ip4conf*)conf->net_proto;	
	
	*fd = socket(PF_INET, SOCK_RAW, conf->ipproto);
	check("raw socket", *fd);

	ret = setsockopt(*fd, IPPROTO_IP, IP_HDRINCL, &one, sizeof(one));
	check("setsockopt (IP_HDRINCL)", ret);
	
	
	dst_addr.sin_family = AF_INET;
	dst_addr.sin_port = 0;
	inet_aton(ipconf->dst, &dst_addr.sin_addr);
	memset(&dst_addr.sin_zero, '0', 8);
		
	ret = connect(*fd, (struct sockaddr*)&dst_addr, sizeof(struct sockaddr));
	check("raw socket (connect)", ret);
	
}



/* Interface for set_nethdr */
void fill_ip4hdr(struct s_sendinfo *sendinfo, struct s_config *conf) {
	struct s_ip4conf *ipconf = conf->net_proto;
	
	struct s_buffer *buffer = &sendinfo->buffer;
	struct ip *iph;
	iph = (struct ip*) get_p(buffer, ipconf->hl * 4);//buffer->buf;

	iph->ip_hl = ipconf->hl;
	iph->ip_v = 4;
	iph->ip_len = htons(buffer->buf_size);
	iph->ip_tos = ipconf->tos;

	iph->ip_id = htons(ipconf->id);
	iph->ip_off = htons(ipconf->off);
	iph->ip_ttl = ipconf->ttl;
	iph->ip_p = *ipconf->protocol;
	iph->ip_sum = 0;

	inet_aton(ipconf->src, &iph->ip_src);
	inet_aton(ipconf->dst, &iph->ip_dst);
	
	iph->ip_sum =  checksum(iph, 20);
	
	//fprintf(stderr, "checksum = %i, %i\n", iph->ip_sum, ipconf->hl * 4);
//	buffer->type = HEADER_INCLUDED;
	
}

void fill_udphdr(struct s_sendinfo *sendinfo, struct s_config *conf) {
	struct s_udpconf *udpconf = conf->trans_proto;
	struct s_buffer *buffer = &(sendinfo->buffer);
	struct udphdr *udph;
	//int i;
	static const int hdrlength = 8;
	udph = (struct udphdr*) get_p(buffer, hdrlength);//(buffer->buf + buffer->buf_size);
	
	
	
	udph->uh_sport = htons(udpconf->sport);
	udph->uh_dport = htons(udpconf->dport);
//	udph->uh_ulen = htons((hdrlength + udpconf->payload_size));
	udph->uh_sum = 0;

	udph->uh_sum = htons( tcp_checksum(IPPROTO_UDP,
						   udph,
						   buffer->buf_size,
						  // hdrlength + udpconf->payload_size,	
						   *udpconf->ip_dst_, 
						   *udpconf->ip_src_) );

		
//	buffer->type = HEADER_INCLUDED;
	
	
}
void fill_icmp6hdr(struct s_sendinfo *sendinfo, struct s_config *conf) {

	struct s_icmp6conf *icmp6conf = conf->trans_proto;
	struct icmp6_hdr *icmp6h;
	struct s_buffer *buffer = &(sendinfo->buffer);
	
	icmp6h = (struct icmp6_hdr*) get_p(buffer, 8);
	
	icmp6h->icmp6_type = icmp6conf->type;
	icmp6h->icmp6_code = icmp6conf->code;
	icmp6h->icmp6_cksum = 0;
	icmp6h->icmp6_id = htons(icmp6conf->id);
	icmp6h->icmp6_seq = htons(icmp6conf->seq);
	
	icmp6h->icmp6_cksum = checksum(icmp6h, buffer->buf_size);
}

void fill_icmphdr(struct s_sendinfo *sendinfo, struct s_config *conf) {
	struct s_icmpconf *icmpconf = conf->trans_proto;
	struct icmp *icmph;
	struct s_buffer *buffer = &(sendinfo->buffer);
	
	icmph = (struct icmp*) get_p(buffer, 8);//(buffer->buf + buffer->buf_size);
	
	icmph->icmp_type = icmpconf->type;
	icmph->icmp_code = icmpconf->code;
	icmph->icmp_cksum = 0;
	
	
	//inet_aton(icmpconf->un.gw_addr, &icmph->icmp_gwaddr);
	//icmph->icmp_pptr = icmpconf->un.pptr;

	icmph->icmp_id = htons(icmpconf->id);
	icmph->icmp_seq = htons(icmpconf->seq);

	if(icmpconf->pptr != 0) {
		icmph->icmp_void = 0;
		icmph->icmp_pptr = icmpconf->pptr;
	}
	if(icmpconf->gw_addr != "") {
		inet_aton(icmpconf->gw_addr, &icmph->icmp_gwaddr);
	}
		
	
	/* put ip buffer + data into the buffer */
	//buffer->buf_size += 20; 
	
	
	icmph->icmp_cksum = checksum(icmph, buffer->buf_size);
	
	//buffer->type = HEADER_INCLUDED;
	
}

/* Interface for set_tcphdr */
void fill_tcphdr(struct s_sendinfo *sendinfo, struct s_config *conf) {
	struct s_tcpconf *tcpconf = conf->trans_proto;
	struct s_buffer *buffer = &(sendinfo->buffer);
	struct tcphdr *tcph;

	
	tcph = (struct tcphdr*) get_p(buffer, tcpconf->off * 4);//(buffer->buf + buffer->buf_size);//(tcppacket->iph->ip_hl * 4));

	tcph->th_sport = htons(tcpconf->sport);
	tcph->th_dport = htons(tcpconf->dport);
	tcph->th_seq = htonl(tcpconf->seq);
	tcph->th_ack = htonl(tcpconf->ack);
	tcph->th_x2 = 0;
	tcph->th_off = tcpconf->off;
	tcph->th_win = htons(tcpconf->win);
	tcph->th_sum = 0;
	tcph->th_urp = htons(tcpconf->urp);

	/* tcp flags */
	if(tcpconf->FIN) tcph->th_flags = tcph->th_flags | TH_FIN;
	if(tcpconf->SYN) tcph->th_flags |= TH_SYN;
	if(tcpconf->RST) tcph->th_flags |= TH_RST;
	if(tcpconf->PUSH)tcph->th_flags |= TH_PUSH;
	if(tcpconf->ACK) tcph->th_flags |= TH_ACK;
	if(tcpconf->URG) tcph->th_flags |= TH_URG;
	

	tcph->th_sum =  tcp_checksum(IPPROTO_TCP,
					 	  tcph,
					 	  buffer->buf_size,	
					 	  *tcpconf->ip_dst_, 
					 	  *tcpconf->ip_src_) ;

	
//	buffer->type = HEADER_INCLUDED;
}



/**************************************************************************
 * 																		  *
 * Functions for ipv6 processing										  *
 * 																		  *
 **************************************************************************/
 
 
void init_rawsocket6(sock_descriptor_t *fd, struct s_config *conf) {
	int ret;
	struct sockaddr_in6 dst_addr;
	struct s_ip6conf *ipconf;
	
	unsigned int tc = 10;
	
	ipconf = (struct s_ip6conf*)conf->net_proto;	
	
	*fd = socket(PF_INET6, SOCK_RAW, conf->ipproto);
	check("raw socket", *fd);

	/* Note: there is nothing like HDRINCL in IPv6. Ancillary data
	   must be used to set the header values of IP packets */
		
	dst_addr.sin6_family = AF_INET6;
	dst_addr.sin6_port = 0;
	inet_pton(AF_INET6, ipconf->dst, &dst_addr.sin6_addr);
	dst_addr.sin6_flowinfo = htonl(tc << 4 | 2);
    dst_addr.sin6_scope_id = if_nametoindex(ipconf->interface);
 	
	ret = connect(*fd, (struct sockaddr*)&dst_addr, sizeof(dst_addr));
	check("raw socket (connect)", ret);

}


void fill_ip6hdr(struct s_sendinfo *sendinfo, struct s_config *conf) {
	struct s_ip6conf *ipconf = conf->net_proto;
	struct cmsghdr *cmsg;
	//struct in6_pktinfo pktinfo;
	char buf[CMSG_SPACE(sizeof(char)) * 1000];
	
	//struct in6_pktinfo *fdptr;
	int *intptr;
	
	//char *testIP = "fe80::30:8475:c8c0";
	
	//sendinfo->msg;
	//inet_pton(AF_INET6, testIP, &pktinfo.ipi6_addr);
	//pktinfo.ipi6_ifindex = if_nametoindex("eth0");	
	
	sendinfo->msg.msg_control = buf;
    sendinfo->msg.msg_controllen = sizeof buf;
    
    /*
    cmsg = CMSG_FIRSTHDR(&sendinfo->msg);
    cmsg->cmsg_level = IPPROTO_IPV6;
	cmsg->cmsg_type = IPV6_PKTINFO;
	cmsg->cmsg_len = CMSG_LEN(sizeof(pktinfo));
	
	fdptr = (struct in6_pktinfo*)CMSG_DATA(cmsg);
	*fdptr = pktinfo;
	*/
	
//	sendinfo->msg.msg_controllen = cmsg->cmsg_len;
	
	//cmsg = CMSG_NXTHDR( &sendinfo->msg, cmsg );
	cmsg = CMSG_FIRSTHDR( &sendinfo->msg );
	cmsg->cmsg_level = IPPROTO_IPV6;
	cmsg->cmsg_type = IPV6_HOPLIMIT;
	cmsg->cmsg_len = CMSG_LEN(sizeof(int));
	intptr = (int*) CMSG_DATA(cmsg);
	*intptr = ipconf->hoplimit;
	
	sendinfo->msg.msg_controllen = cmsg->cmsg_len;
	
	cmsg = CMSG_NXTHDR( &sendinfo->msg, cmsg );
	
	// Build the packet..
	
	struct s_buffer *buffer = &(sendinfo->buffer);
	struct ip6_hdr *ip6h;

	
	ip6h = (struct ip6_hdr*) get_p(buffer, 40);
	ip6h->ip6_hops = ipconf->hoplimit;
	ip6h->ip6_vfc = 6 << 4;
	ip6h->ip6_nxt = conf->ipproto;
	ip6h->ip6_plen = htons(buffer->buf_size - 40);
	
	inet_pton(AF_INET6, ipconf->src, &ip6h->ip6_src);
	inet_pton(AF_INET6, ipconf->dst, &ip6h->ip6_dst);
	
	buffer->buf_size -= 40;
	buffer->buf += 40;
}


char* get_p(struct s_buffer *buffer, int size) {
	
	if(buffer->buf_size + size > buffer->allocated_space)
		return NULL;
	
	buffer->buf -= size;
	buffer->buf_size += size;
	
	
	return buffer->buf;
}







//------------------------------------------------------
//------------------------------------------------------------------


/* we have to specify the header values by using ancillary data */
void __fill_ip6hdr(struct s_sendinfo *sendinfo, struct s_config *conf) {
	//struct s_ip6conf *ipconf = conf->net_proto;
	//struct msghdr msg;
	struct cmsghdr *cmsgptr;
	struct in6_pktinfo pktinfo;
	
	char *testIP = "::1";
	
	//unsigned char ctlbuf[1000];
	char *ctlbuf;
	int* descp;
	struct in6_pktinfo *datapktinfo;
	
	
	inet_pton(AF_INET6, testIP, &pktinfo.ipi6_addr);
	pktinfo.ipi6_ifindex = if_nametoindex("eth0");	
	
	fprintf(stderr, "DEBUG_1\n");

	//TODO check how much buffer should be malloced 
	ctlbuf = (char*) cmalloc(CMSG_SPACE(sizeof(char) * 1000));
	
	
	
	sendinfo->msg.msg_control = ctlbuf;
	sendinfo->msg.msg_controllen = sizeof ctlbuf;
	
	//msg.msg_control = ctlbuf;
	//msg.msg_controllen = sizeof ctlbuf;
	
	fprintf(stderr, "DEBUG_2\n");

	cmsgptr = CMSG_FIRSTHDR( &sendinfo->msg );
	//cmsgptr = CMSG_FIRSTHDR( &msg );
	
	fprintf(stderr, "DEBUG_2.1\n");
	/* specify hop limit*/
	cmsgptr->cmsg_len = CMSG_LEN( sizeof(int) );
	fprintf(stderr, "DEBUG_2.2\n");
	cmsgptr->cmsg_level = IPPROTO_IPV6;
	cmsgptr->cmsg_type = IPV6_HOPLIMIT;
	descp = (int*) CMSG_DATA( cmsgptr );
	*descp = 20; 
	cmsgptr = CMSG_NXTHDR( &sendinfo->msg, cmsgptr );
	//cmsgptr = CMSG_NXTHDR( &msg, cmsgptr );
	
	fprintf(stderr, "DEBUG_3\n");
	/* specify traffic class */
	/*
	cmsgptr->cmsg_len = CMSG_LEN( sizeof(int) );
	cmsgptr->cmsg_level = IPPROTO_IPV6;
	cmsgptr->cmsg_type = IPV6_TCLASS;
	descp = (int*) CMSG_DATA( cmsgptr );
	*descp = 2; 
	cmsgptr = CMSG_NXTHDR( &msg, cmsgptr );
	*/
	
	/* specify destination IP and outgoing interface */
	cmsgptr->cmsg_len = CMSG_LEN( sizeof(int) );
	cmsgptr->cmsg_level = IPPROTO_IPV6;
	cmsgptr->cmsg_type = IPV6_PKTINFO;
	datapktinfo = (struct in6_pktinfo*) CMSG_DATA( cmsgptr );
	*datapktinfo = pktinfo; 
	cmsgptr = CMSG_NXTHDR( &sendinfo->msg, cmsgptr );
	//cmsgptr = CMSG_NXTHDR( &msg, cmsgptr );

	fprintf(stderr, "DEBUG_4\n");
	
	
	if( !cmsgptr )
		fprintf(stderr, "proto_startup(): ctl is NULL" );
	
	//sendinfo->msg.msg_controllen = (char*)cmsgptr - (char*) ctlbuf;
	//msg.msg_controllen = (char*)cmsgptr - (char*) ctlbuf;
	fprintf(stderr, "DEBUG_5\n");
}
//void fill_tcphdr(struct s_packet *buffer, struct s_tcpconf *tcpconf) {
//}






