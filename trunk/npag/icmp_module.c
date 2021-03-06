/*
 * This file is part of
 * npag - Network Packet Generator
 * Copyright (C) 2005 Christian Bannes, University of T�bingen,
 * Germany
 * 
 * npag is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, 
 * MA  02110-1301, USA.
 */

#define _BSD_SOURCE
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/ip_icmp.h>
#include <netinet/icmp6.h>

#include <netinet/in.h> //IPPROTO
#include <arpa/inet.h> //inet_pton
#include <string.h> //memset
#include <net/if.h> //map interface to index
#include "sendinfo.h"
#include "packet_buffer.h"
#include "config.h"

#define SIN6_LEN //required for compile-time tests

void 	__P(check(char *msg, int c));
u_int16_t 	__P(tcp_checksum());
void 			__P(set_payload());
void 			__P(check_warning());
typedef int sock_descriptor_t;
int checksum 	__P((u_int16_t *buf, int nbytes));


void fill_icmp6hdr(packet_buffer_t *sendinfo, config_t *conf) {

	icmp6conf_t *icmp6conf = conf->trans_proto;
	struct icmp6_hdr *icmp6h;
	packet_buffer_t *buffer = (sendinfo);
	
	icmp6h = (struct icmp6_hdr*) p_buf_get_data_ptr(buffer, 8);
	
	icmp6h->icmp6_type = icmp6conf->type;
	icmp6h->icmp6_code = icmp6conf->code;
	icmp6h->icmp6_cksum = 0;
	icmp6h->icmp6_id = htons(icmp6conf->id);
	icmp6h->icmp6_seq = htons(icmp6conf->seq);
	
	icmp6h->icmp6_cksum = checksum((u_int16_t*)icmp6h, buffer->data_size);
}

void fill_icmphdr(packet_buffer_t *sendinfo, config_t *conf) {
	icmpconf_t *icmpconf = conf->trans_proto;
	struct icmp *icmph;
	packet_buffer_t *buffer = (sendinfo);
	
	icmph = (struct icmp*) p_buf_get_data_ptr(buffer, 8);
	
	icmph->icmp_type = icmpconf->type;
	icmph->icmp_code = icmpconf->code;
	icmph->icmp_cksum = 0;
	
	icmph->icmp_id = htons(icmpconf->id);
	icmph->icmp_seq = htons(icmpconf->seq);

	if(icmpconf->pptr != 0) {
		icmph->icmp_void = 0;
		icmph->icmp_pptr = icmpconf->pptr;
	}
	if(icmpconf->gw_addr != "") {
		inet_aton(icmpconf->gw_addr, &icmph->icmp_gwaddr);
	}
		
	
	icmph->icmp_cksum = checksum((u_int16_t*)icmph, buffer->data_size);

}


