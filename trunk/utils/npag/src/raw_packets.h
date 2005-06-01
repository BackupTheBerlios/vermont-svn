

typedef int sock_descriptor_t;
//#include "npag.h"

void init_rawsocket		__P((sock_descriptor_t *fd, struct s_config*));
void init_rawsocket6	__P((sock_descriptor_t *fd, struct s_config*));

void fill_ip4hdr		__P((struct s_sendinfo*, struct s_config*));
void fill_ip6hdr		__P((struct s_sendinfo*, struct s_config*));
void fill_tcphdr		__P((struct s_sendinfo*, struct s_config*));
void fill_icmphdr		__P((struct s_sendinfo*, struct s_config*));
void fill_icmp6hdr		__P((struct s_sendinfo*, struct s_config*));
void fill_udphdr		__P((struct s_sendinfo*, struct s_config*));

void init_tcpsocket		__P((sock_descriptor_t *fd, struct s_config*));
void init_tcpip6socket	__P((sock_descriptor_t *fd, struct s_config*));
void init_udpsocket		__P((sock_descriptor_t *fd, struct s_config*));
void init_udpip6socket	__P((sock_descriptor_t *fd, struct s_config*));

void set_ipsockopts		__P((struct s_sendinfo*, struct s_config*));
void set_tcpsockopts	__P((struct s_sendinfo*, struct s_config*));
void set_udpsockopts	__P((struct s_sendinfo*, struct s_config*));

char* get_p				__P((struct s_buffer *buffer, int size));
