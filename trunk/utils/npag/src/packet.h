
//enum s_packet_type{HEADER_INCLUDED, HEADER_NOT_INCLUDED, NOT_DEFINED};
//typedef enum s_packet_type e_packet_type;
enum proto {PROTO_TCP, PROTO_UDP};

/*
 * the packet which we want so send. In case of raw sockets the buffer contains
 * the protocol header. Otherwise it is just the payload.
 * The type parameter specifies if the buffer has the protocol header included (i.e. raw sockets are used)
 * */
struct s_buffer {
	char *buf;
	int buf_size;
	
	int allocated_space;
	char *begin;
	
	//this should be deleted..
/*
	int nethdr_size;
	int transhdr_size;
*/
//	e_packet_type type;
	int proto;
};

/*
struct s_cksuminfo {
	char *pseudohdr; 
	int pseudohdr_size;
	u_int16_t *total_packet_length; 
	u_int16_t *cksum; 
};
*/

/*
 * Information needed to send a packet:
 * 1. a socket descriptor
 * 2. ancillary data for ip header specification
 * 3. the packet which might contain the header (in case of raw sockets)
 *
 */
struct s_sendinfo {
	int *fd; /* socket descriptor */
	struct msghdr msg;	/* ancilarry data */
//	struct s_cksuminfo cksuminfo;
	struct s_buffer buffer;
//	u_int16_t *udp_length;
};


