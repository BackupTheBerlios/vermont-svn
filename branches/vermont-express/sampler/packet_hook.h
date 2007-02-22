#ifndef PACKET_HOOK_H
#define PACKET_HOOK_H

struct packet_hook {

        /* full packet data w/ link layer */
        void *data;

        /* pointer to IP header; data+phy.offset */
        void *ip_header;

        /* pointer to transport header; IPHeader+IPHeaderLength */
        void *transport_header;

        /* time when this packet was captured */
        const struct timeval *timestamp;

        /* length of packet in bytes, including link layer header */
	int length;

	/* classification of the packet. can be PCLASS_NET_XX or PCLASS_TRN_XXX. See Packet.h*/
	int classification;
};



#endif
