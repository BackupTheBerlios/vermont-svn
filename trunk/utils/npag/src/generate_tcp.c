
#define _BSD_SOURCE


#include <sys/socket.h>
#include <stdio.h>
#include <netinet/tcp.h>
#include <netinet/ip.h>
#include <arpa/inet.h> //inet_aton
#include <string.h> //memset
#include <netinet/in.h> //ipv6
#include <arpa/inet.h> //inet_pton
#include <net/if.h> //map interface to index

#include "packet.h"
#include "config.h"

#define SIN6_LEN //required for compile-time tests

//typedef int sock_descriptor_t;

void check(char *msg, int c);
//void check_warning(char *msg, int c);
u_int16_t tcp_checksum();
void set_payload();
void check_warning();
typedef int sock_descriptor_t;

//TODO check warnings for setsockopt


void set_ipsockopts(struct s_sendinfo* sendinfo, struct s_config *conf){
	struct s_ip4conf* ipconf = conf->net_proto;
//	fprintf(stderr, "ipsockopts\n");
	int ret; 
//	sendinfo->buffer.type = HEADER_NOT_INCLUDED;
	
	ret = setsockopt(*sendinfo->fd, IPPROTO_IP, IP_TOS, &ipconf->tos, sizeof(int));
	check_warning("WARNING IP tos option", ret);
	ret = setsockopt(*sendinfo->fd, IPPROTO_IP, IP_TTL, &ipconf->ttl,sizeof(int));
	check_warning("WARNING IP ttl option", ret);
}


void set_tcpsockopts(struct s_sendinfo* sendinfo, struct s_config *conf){
	struct s_tcpconf* tcpconf = conf->trans_proto;
	int ret;
//	sendinfo->buffer.type = HEADER_NOT_INCLUDED;
	sendinfo->buffer.proto = PROTO_TCP;
	//int i;
	
	/* save payload into buffer */
	struct s_buffer *buffer = &(sendinfo->buffer);
	//for(i = 0; i < tcpconf->payload_size; ++i)	{
	//	*(buffer->buf + buffer->bufsize + i) = (rand() % 70) + 50;
	//}

	set_payload(buffer->buf, tcpconf->payload_size, tcpconf->pfile, 0);
	buffer->buf_size += tcpconf->payload_size;
	
	ret = setsockopt(*sendinfo->fd, IPPROTO_TCP, TCP_MAXSEG, &tcpconf->mss, sizeof(int));
	check_warning("WARNING TCP mss option", ret);
	ret = setsockopt(*sendinfo->fd, IPPROTO_TCP, TCP_NODELAY, &tcpconf->nodelay, sizeof(int));
	check_warning("WARNING TCP nodelay option", ret); 
	/*setsockopt(sendinfo->fd, IPPROTO_TCP, TCP_MAXRT, &ipconf->ttl, &sint);*/
	
	
}
void set_udpsockopts(struct s_sendinfo* sendinfo, struct s_config *conf){
	struct s_udpconf* udpconf = conf->trans_proto;
//	sendinfo->buffer.type = HEADER_NOT_INCLUDED;
	sendinfo->buffer.proto = PROTO_UDP;
	//int i;
		/* save payload into buffer */
	struct s_buffer *buffer = &(sendinfo->buffer);
	//for(i = 0; i < udpconf->payload_size; ++i)	{
	//	*(buffer->buf + buffer->bufsize + i) = (rand() % 70) + 50;
	//}
	set_payload(buffer->buf, udpconf->payload_size, udpconf->pfile, 0);
	buffer->buf_size += udpconf->payload_size;
}



void init_tcpsocket(sock_descriptor_t *fd, struct s_config *conf){
	int ret;
	int one = 1;
	struct sockaddr_in dst_addr;
	struct sockaddr_in src_addr;

	struct s_ip4conf *ipconf;
	struct s_tcpconf *tcpconf;

	ipconf = (struct s_ip4conf*)conf->net_proto;
	tcpconf = (struct s_tcpconf*)conf->trans_proto;

	*fd = socket(PF_INET, SOCK_STREAM, 0);
	check("tcp socket", *fd);
	
	ret = setsockopt(*fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
	check("tcp setsockopt(SO_REUSEADDR)", ret);
	
	/*int ttl = 20;
	fprintf(stderr, "fd = %i\n", *fd);	
	if(setsockopt(*fd, IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl)) == -1) {
		fprintf(stderr, "not supported\n");	
	}*/
	
	dst_addr.sin_family = AF_INET;
	dst_addr.sin_port = htons(tcpconf->dport);
	inet_aton(ipconf->dst, &dst_addr.sin_addr);
	memset(&dst_addr.sin_zero, '0', 8);

	src_addr.sin_family = AF_INET;
	src_addr.sin_port = htons(tcpconf->sport);
	inet_aton(ipconf->src, &src_addr.sin_addr);
	memset(&src_addr.sin_zero, '0', 8);

	ret = bind(*fd, (struct sockaddr*)&src_addr, sizeof(struct sockaddr));
	check("tcp bind", ret);

	ret = connect(*fd, (struct sockaddr*)&dst_addr, sizeof(struct sockaddr));
	check("tcp connect", ret);
}

void init_udpsocket(sock_descriptor_t *fd, struct s_config *conf){
	int ret;
	int one = 1;
	struct sockaddr_in dst_addr;
	struct sockaddr_in src_addr;

	struct s_ip4conf *ipconf;
	struct s_udpconf *udpconf;

	ipconf = (struct s_ip4conf*)conf->net_proto;
	udpconf = (struct s_udpconf*)conf->trans_proto;

	*fd = socket(PF_INET, SOCK_DGRAM, 0);
	check("udp socket", *fd);
	
	ret = setsockopt(*fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
	check("udp setsockopt(SO_REUSEADDR)", ret);

	dst_addr.sin_family = AF_INET;
	dst_addr.sin_port = htons(udpconf->dport);
	inet_aton(ipconf->dst, &dst_addr.sin_addr);
	memset(&dst_addr.sin_zero, '0', 8);

	src_addr.sin_family = AF_INET;
	src_addr.sin_port = htons(udpconf->sport);
	inet_aton(ipconf->src, &src_addr.sin_addr);
	memset(&src_addr.sin_zero, '0', 8);

	ret = bind(*fd, (struct sockaddr*)&src_addr, sizeof(struct sockaddr));
	check("udp bind", ret);

	ret = connect(*fd, (struct sockaddr*)&dst_addr, sizeof(struct sockaddr));
	check("udp connect", ret);
}



 
/***
 *  TODO sin6_scope_id is now set to eth0 but the value should
 * be generated dynamicaly!!
 ***/
 void init_tcpip6socket(sock_descriptor_t *fd, struct s_config *conf){

	int ret;
	int one = 1;
	struct sockaddr_in6 dst_addr;
	struct sockaddr_in6 src_addr;
	
	
	struct s_ip6conf *ipconf;
	struct s_tcpconf *tcpconf;
	
	ipconf = (struct s_ip6conf*)conf->net_proto;
	tcpconf = (struct s_tcpconf*)conf->trans_proto;
	
	*fd = socket(AF_INET6, SOCK_STREAM, 0);
	check("tcp socket", *fd);
	
	ret = setsockopt(*fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
	check("tcp setsockopt(SO_REUSEADDR)", ret);
	
	dst_addr.sin6_family = AF_INET6;
	dst_addr.sin6_port = htons(tcpconf->dport);
	inet_pton(AF_INET6, ipconf->dst, &dst_addr.sin6_addr);
	dst_addr.sin6_flowinfo = 0;
    dst_addr.sin6_scope_id = if_nametoindex("eth0");
	
	
	src_addr.sin6_family = AF_INET6;
	src_addr.sin6_port = htons(tcpconf->sport);
	inet_pton(AF_INET6, ipconf->src, &src_addr.sin6_addr);
	src_addr.sin6_flowinfo = 0;
	

	ret = connect(*fd, (struct sockaddr*)&dst_addr, sizeof(dst_addr));
	check("tcp connect", ret);
}


void init_udpip6socket(sock_descriptor_t *fd, struct s_config *conf){

	int ret;
	int one = 1;
	struct sockaddr_in6 dst_addr;
	struct sockaddr_in6 src_addr;
	
	
	struct s_ip6conf *ipconf;
	struct s_udpconf *udpconf;
	
	ipconf = (struct s_ip6conf*)conf->net_proto;
	udpconf = (struct s_udpconf*)conf->trans_proto;
	
	*fd = socket(AF_INET6, SOCK_STREAM, 0);
	check("udp socket", *fd);
	
	ret = setsockopt(*fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
	check("udp setsockopt(SO_REUSEADDR)", ret);
	
	dst_addr.sin6_family = AF_INET6;
	dst_addr.sin6_port = htons(udpconf->dport);
	inet_pton(AF_INET6, ipconf->dst, &dst_addr.sin6_addr);
	dst_addr.sin6_flowinfo = 0;
    dst_addr.sin6_scope_id = if_nametoindex("eth0");
	
	
	src_addr.sin6_family = AF_INET6;
	src_addr.sin6_port = htons(udpconf->sport);
	inet_pton(AF_INET6, ipconf->src, &src_addr.sin6_addr);
	src_addr.sin6_flowinfo = 0;
	

	ret = connect(*fd, (struct sockaddr*)&dst_addr, sizeof(dst_addr));
	check("udp connect", ret);
}

