#define _BSD_SOURCE

#include <stdio.h>
#include <pthread.h>
#include <stdlib.h> //malloc
#include <sys/socket.h>
#include <string.h> //memset

#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>

#include <arpa/inet.h> //inet_aton
#include <sys/uio.h> //struct iovec

#include <sys/time.h>
//#include <sys/types.h>
#include <unistd.h> //get_pid



#include "memory.h"
#include "config.h"
#include "packet.h"
#include "raw_packets.h"
#include "npag.h"




int set_payload(char *buf, int size, char *filename, int format, int pos);

struct s_generator {
	void (*set_nethdr)(struct s_sendinfo*, struct s_config*);
	void (*set_transhdr)(struct s_sendinfo*, struct s_config*);
	void (*init_socket) (sock_descriptor_t *fd, struct s_config *conf);
};


struct s_adhdr {
	int payload_size;
	struct timeval tv;
	int seq;
	
	unsigned short mysize;
};

void set_nethdr_NOT_DEFINED		__P(());
void set_transhdr_NOT_DEFINED	__P(());
void init_socket_NOT_DEFINED	__P(());
void sectotv				__P((double sec, struct timeval *tv));
void wait_timeval			__P((struct timeval *tv));
void init_generator			__P((struct s_generator *gen, struct s_config *conf));
void send_raw				__P((struct s_sendinfo*, struct s_config*, struct s_generator*));
void send_kernel			__P((struct s_sendinfo*, struct s_config*, struct s_generator*));
void init_sendinfo			__P((struct s_sendinfo *sendinfo));


extern int verbose;
extern int num_of_threads;
extern FILE *output;
extern int out;
extern pthread_cond_t master_wait;


/************************************************************
 * Actual code starts here									*
 ************************************************************/

void* exec_thread(void *p) {
	struct s_config *conf = p;
	
	struct s_sendinfo sendinfo;
	struct s_generator gen = {set_nethdr_NOT_DEFINED, set_transhdr_NOT_DEFINED, init_socket_NOT_DEFINED};

	init_sendinfo(&sendinfo);
	init_generator(&gen, conf);
	
	gen.init_socket(sendinfo.fd, conf);
		
	if(conf->kernel_packet == FALSE) {
		//fprintf(stderr, "send_raw\n");
		send_raw(&sendinfo, conf, &gen);
	}
	else {
		send_kernel(&sendinfo, conf, &gen);	
	}
	
	/* TODO change this */
	return NULL;
}



void send_raw(struct s_sendinfo *sendinfo, struct s_config *conf,  struct s_generator *gen) {

	int i, j, k;
	int nbytes;
	struct sockaddr_in *dst_addr = NULL;
	int sizeof_dst_addr = 0;
	struct iovec msg_iov;
	struct msghdr msgh;
	
	struct timeval tv, delay_tv;
	struct s_buffer *buffer = &sendinfo->buffer;
	struct s_pattern *pattern;
	sock_descriptor_t fd = *sendinfo->fd;
	
	struct s_traffic *	traffic = &conf->traffic;	


	/* fill in msgh */	 
	msgh.msg_name = (struct sockaddr*)dst_addr; /* destination address */
	msgh.msg_namelen = sizeof_dst_addr;  /* address length */  
	msgh.msg_iov = &msg_iov;		     /* message buffer */
	msgh.msg_iovlen = 1;			     /* number of message buffers */
	//msgh.msg_control = NULL;		     /* ancillary data */
	//msgh.msg_controllen = 0;
	msgh.msg_control = sendinfo->msg.msg_control;
	msgh.msg_controllen = sendinfo->msg.msg_controllen;;
	msgh.msg_flags = 0;
	
			
	//next traffic pattern
	for(k = 0; k < traffic->repeat + 1; k++) {
//	fprintf(stderr, "traffic repeat loop\n");
	pattern = traffic->pattern;
	while(pattern != NULL) {		
//		fprintf(stderr, "pattern->next loop\n");
		sectotv(pattern->delay, &delay_tv);
					
		/* set payload */
		set_payload(get_p(buffer, pattern->payload_size), pattern->payload_size, pattern->payload_file, pattern->file_format, 0);
		//fprintf(stderr, "payload build\n");
		gen->set_transhdr(sendinfo, conf);
		//fprintf(stderr, "trans build\n");
		gen->set_nethdr(sendinfo, conf);
		//fprintf(stderr, "net build\n");	
			
		msgh.msg_control = sendinfo->msg.msg_control;
		msgh.msg_controllen = sendinfo->msg.msg_controllen;;
			
		msg_iov.iov_len = buffer->buf_size;
			
		//repeat a traffic pattern
		for(j = 0; j < pattern->repeat + 1; ++j) {
			tv = delay_tv;
			
			//send traffic
			if(verbose)
				fprintf(stdout,"Thread %i starts sending...\n",getpid());
			for(i = 0; i < pattern->send_packets; ++i) {
				msg_iov.iov_base = buffer->buf;
					nbytes = sendmsg(fd,&msgh, 0  );
					
					
					if(out) {
						if(conf->network == TYPE_IP6) {
							//fprintf(stderr, "extending buffer\n");	
							buffer->buf_size += 40;
							buffer->buf -= 40;	
						}
						unsigned char val;
						char str[2];
						int ii;
						for(ii = 0; ii < buffer->buf_size; ++ii) {
							val = *(buffer->buf + ii);
							//fprintf(stderr, "%i ",val );
							sprintf(str, "%x ", val);
							if(strlen(str) == 2) fputc('0', output);
							fputs(str, output);
							//if((ii + 1) % 16 == 0 && (ii + 1) < buffer->buf_size) fputc('\n', output);
						}
						fputs("\n\n", output);
						if(conf->network == TYPE_IP6) {
							//fprintf(stderr, "reducing buffer\n");
							buffer->buf_size -= 40;
							buffer->buf += 40;	
						}
					}
					
					if(verbose > 1)
						printf("%i: %i bytes written\n", getpid(),nbytes);
				//printf("%i bytes written\n", nbytes);
			}
			if(verbose)
				fprintf(stdout,"Thread %i is waiting %f seconds\n",getpid(), pattern->delay);
			wait_timeval(&tv);
		}
			pattern = pattern->next;
	}//end while
	if(k  != traffic->repeat  && verbose > 0)
		fprintf(stdout, "THREAD %i IS REPEATING THE TRAFFIC\n", getpid());
	}//endfor
	
	
	if(verbose)
		fprintf(stdout,"Thread %i HAS FINISHED ITS WORK\n",getpid());
	
	
	num_of_threads--; //TODO I think I need a mutex lock here...	
	if(num_of_threads == 0) {
		//fprintf(stderr, "sending broadcast: \n"); 
		pthread_cond_broadcast(&master_wait);	
	}
		
	/* free memoy */
	if(dst_addr != NULL)
		cfree(dst_addr);
}


void send_kernel(struct s_sendinfo *sendinfo, struct s_config *conf, struct s_generator *gen) {

	int i, j, l;
	int nbytes;
	char sndbuf[65536];
	struct timeval tv;
	struct timeval delay_tv;
	//struct udp_addhdr udp_ah;
	struct s_adhdr ah;
	struct s_buffer *buffer = &sendinfo->buffer;

	struct s_pattern *pattern;
	
	struct sockaddr_in *dst_addr = NULL;
	int sizeof_dst_addr = 0;
	struct iovec msg_iov;
	struct msghdr msgh;
	
	sock_descriptor_t fd = *sendinfo->fd;
	struct s_traffic *traffic = &conf->traffic;
	
	
	gen->set_transhdr(sendinfo, conf);
	gen->set_nethdr(sendinfo, conf);
	
	
		/* fill in msgh */	 
	msgh.msg_name = (struct sockaddr*)dst_addr; /* destination address */
	msgh.msg_namelen = sizeof_dst_addr;  /* address length */  
	msgh.msg_iov = &msg_iov;		     /* message buffer */
	msgh.msg_iovlen = 1;			     /* number of message buffers */
	//msgh.msg_control = NULL;		     /* ancillary data */
	//msgh.msg_controllen = 0;
	msgh.msg_control = sendinfo->msg.msg_control;
	msgh.msg_controllen = sendinfo->msg.msg_controllen;;
	msgh.msg_flags = 0;

	
	if(sendinfo->buffer.proto == PROTO_UDP) {
		ah.mysize = sizeof(int) * 2 + sizeof(struct timeval);
		//connection establish
	 	msg_iov.iov_len =  ah.mysize;//sizeof(struct udp_addhdr);
	 	ah.seq = 0;
	 	msg_iov.iov_base = &ah;
	 	sendmsg(fd,&msgh, 0  );
	}
		
	else if (sendinfo->buffer.proto == PROTO_TCP) {
		ah.mysize = sizeof(int) + sizeof(struct timeval);
	}
		
	int udp_seq = 2;
	for(l = 0; l < traffic->repeat + 1; l++) {
		//fprintf(stderr, "traffic_repeat..\n");
		pattern = traffic->pattern;
		while(pattern != NULL) {
			int buf_size;
	
			set_payload(get_p(buffer, pattern->payload_size), pattern->payload_size, pattern->payload_file, pattern->file_format, 0);
			buf_size = buffer->buf_size * pattern->send_packets;
			buf_size += ah.mysize * pattern->send_packets;
		
			msg_iov.iov_len = buffer->buf_size + ah.mysize;
			sectotv(pattern->delay, &delay_tv);
				for(j = 0; j < pattern->repeat + 1; ++j) {
					tv = delay_tv; //tv is overwritten by wait_timeval in LINUX
	 				i = 0;
	 			
	 			/*	
 				while(i < buf_size) {
 					int k = 0;
 					i += ah.mysize;
 			
 					// invariant: i is a multiple of (payloadsize + ahsize = total payloadsize)
 				
 					//TODO use memcopy instead of this loop
 					fprintf(stderr, "buf_size = %i, i = %i\n", buf_size, i);
 					for(k = 0; k < buffer->buf_size; k++) {
 						//sndbuf[i + k] = (rand() % 70) + 50;
						sndbuf[i + k] = *(buffer->buf + k);	
						fprintf(stderr, "i+k = %i, k = %i\n", (i + k), k);
					}
 					i += k;
 				}*/
 				
 					/* set the payload. Leave some space for additional header */
 					memcpy(sndbuf + ah.mysize, buffer->buf, buffer->buf_size);
 				
 				
	 				// send packets
 					ah.payload_size = buffer->buf_size + ah.mysize;
 					if(verbose)
						fprintf(stdout,"Thread %i starts sending...\n",getpid());
 					for(i = 0; i < pattern->send_packets; ++i) {
 				
 					//TODO you need a mutex here..suppose the sceduler
 					// decides to start another thread right after you asked
 					// for the current time. The result will be a fault 
 					// time value!! Asking for the current time and sending
 					// this time MUST be atomic!
 				
 						gettimeofday(&ah.tv, NULL);
						ah.seq = udp_seq++;
			
 						//memcpy(sndbuf + i * ah.payload_size, &ah, ah.mysize);
 						memcpy(sndbuf, &ah, ah.mysize);
 				
 						//msg_iov.iov_base = sndbuf + i * ah.payload_size;
 						//fprintf(stderr, "%i  ", ah.payload_size );
 						msg_iov.iov_base = sndbuf;
 						nbytes = sendmsg(fd,&msgh, 0  );
 		
						if(nbytes == -1) perror("send");
						if(verbose > 1)
							printf("%i: %i bytes written\n", getpid(),nbytes);
 					}
 				
 				//	if(j < pattern->repeat) {
 						if(verbose)
							fprintf(stdout,"Thread %i is waiting %f seconds\n",getpid(), pattern->delay);
 				
 						wait_timeval(&tv);
 				//	}
 					
				}//repeat pattern
				pattern = pattern->next;
				
		}//next pattern
		if(l  != traffic->repeat  && verbose > 0) 
			fprintf(stdout, "THREAD %i IS REPEATING THE TRAFFIC\n", getpid());
		
		
	}//endfor traffic repeat
	
		//UDP connection teardown
	if(sendinfo->buffer.proto == PROTO_UDP) {
		fprintf(stdout, "UDP connection teardown...\n");
		fflush(stdout);
		
		sectotv(2.0, &tv);
		wait_timeval(&tv);
	 	
	 	msg_iov.iov_len =  ah.mysize;//sizeof(struct udp_addhdr);
	 	ah.seq = udp_seq; //last sequence number (+1) that was sent
	 	msg_iov.iov_base = &ah;
	 	sendmsg(fd,&msgh, 0  );
	 	
	 	sectotv(2.0, &tv);
		wait_timeval(&tv);
	 	
	 	ah.seq = 1;
	 	msg_iov.iov_base = &ah;
	 	sendmsg(fd,&msgh, 0  );
	}
	
	num_of_threads--; //TODO I think I need a mutex lock here...	
	if(num_of_threads == 0) {
		//fprintf(stderr, "sending broadcast: \n"); 
		pthread_cond_broadcast(&master_wait);	
	}
}


void _send_kpackets(sock_descriptor_t fd, struct s_sendinfo *sendinfo, struct s_traffic *traffic, struct msghdr *_msgh, struct iovec *_msg_iov) {

	int i, j, l;
	int nbytes;
	char sndbuf[65536];
	struct timeval tv;
	struct timeval delay_tv;
	//struct udp_addhdr udp_ah;
	struct s_adhdr ah;
	struct s_buffer *buffer = &sendinfo->buffer;

	struct s_pattern *pattern;


	struct msghdr *msgh = _msgh;
	struct iovec *msg_iov = _msg_iov;
		
		if(sendinfo->buffer.proto == PROTO_UDP) {
			ah.mysize = sizeof(int) * 2 + sizeof(struct timeval);
			//connection establish
		 	msg_iov->iov_len =  ah.mysize;//sizeof(struct udp_addhdr);
		 	ah.seq = 0;
		 	msg_iov->iov_base = &ah;
		 	sendmsg(fd,msgh, 0  );
		}
		
		else if (sendinfo->buffer.proto == PROTO_TCP) {
			ah.mysize = sizeof(int) + sizeof(struct timeval);
		}
		
		int udp_seq = 2;
		for(l = 0; l < traffic->repeat; l++) {
		pattern = traffic->pattern;
		while(pattern != NULL) {
			int buf_size;
			//struct udp_addhdr udp_ah;
			//int ah_size = ah.mysize;//sizeof(struct udp_addhdr);
			//int udp_seq = 2;
			//buf_size = traffic->packet_size * traffic->send_packets;
			//buffer->buf_size = set_payload(buffer->buf, pattern->payload_size, pattern->payload_file, 0);
			set_payload(get_p(buffer, pattern->payload_size), pattern->payload_size, pattern->payload_file, pattern->file_format, 0);
			buf_size = buffer->buf_size * pattern->send_packets;
			buf_size += ah.mysize * pattern->send_packets;
	

			//msg_iov.iov_len = traffic->packet_size;
			//msg_iov->iov_len = traffic->packet_size + sizeof(struct udp_addhdr);
			//msg_iov->iov_len = traffic->packet_size + ah.mysize;
			msg_iov->iov_len = buffer->buf_size + ah.mysize;
			sectotv(pattern->delay, &delay_tv);

			for(j = 0; j < pattern->repeat + 1; ++j) {
				tv = delay_tv; //tv is overwritten by wait_timeval in LINUX

	 			i = 0;
	 			while(i < buf_size) {
	 				int k = 0;
	 				i += ah.mysize;//sizeof(struct udp_addhdr);
	 			//	for(k = 0; k < traffic->packet_size - ah_size ; k++) {
	 				// invariant: i is a multiple of (payloadsize + ahsize = total payloadsize)
	 				
	 				//TODO use memcopy instead of this loop
	 				//fprintf(stderr, "buffer contains: %s\n", buffer->buf);
	 				//for(k = 0; k < buffer->bufsize - ah_size ; k++) {
	 				for(k = 0; k < buffer->buf_size; k++) {
	 					//sndbuf[i + k] = (rand() % 70) + 50;
						sndbuf[i + k] = *(buffer->buf + k);	
					}
	 				i += k;
	 			}

	 			// send packets
	 			//ah.payload_size = traffic->packet_size + ah.mysize;//sizeof(struct udp_addhdr);
	 			ah.payload_size = buffer->buf_size + ah.mysize;
	 			if(verbose)
					fprintf(stdout,"Thread %i starts sending...\n",getpid());
	 			for(i = 0; i < pattern->send_packets; ++i) {
	 				
	 				//TODO you need a mutex here..suppose the sceduler
	 				// decides to start another thread right after you asked
	 				// for the current time. The result will be a fault 
	 				// time value!! Asking for the current time and sending
	 				// this time MUST be atomic!
	 				
	 				gettimeofday(&ah.tv, NULL);
					ah.seq = udp_seq++;
					//ah.seq = udp_seq--;
					
	 				//printf("length = %i, \ttime = %ld  and %ld\n", udp_ah.length, udp_ah.tv.tv_sec, udp_ah.tv.tv_usec);
					/*
					printf("copy hdr at %i\n", i * traffic->packet_size );
	 				memcpy(sndbuf + i * traffic->packet_size, &ah, ah.mysize);//sizeof(struct udp_addhdr));
	 				msg_iov->iov_base = sndbuf + i * traffic->packet_size;
	 				*/
	 				
	 				//printf("copy hdr at %i\n", i * ah.payload_size );
	 				//memcpy(sndbuf + i * buffer->bufsize, &ah, ah.mysize);//sizeof(struct udp_addhdr));
	 				memcpy(sndbuf + i * ah.payload_size, &ah, ah.mysize);
	 				//msg_iov->iov_base = sndbuf + i * buffer->bufsize;
	 				msg_iov->iov_base = sndbuf + i * ah.payload_size;
	 				nbytes = sendmsg(fd,msgh, 0  );
	 	
					if(nbytes == -1) perror("send");
					if(verbose)
						printf("%i: %i bytes written\n", getpid(),nbytes);
	 			}
	 			if(verbose)
					fprintf(stdout,"Thread %i is waiting %f seconds\n",getpid(), pattern->delay);
	 			wait_timeval(&tv);


				//printf("Repeating pattern with different payload:\n");
			}
			pattern = pattern->next;
		}
		if(l + 1 != traffic->repeat)
		fprintf(stdout, "\nREPEATING TRAFFIC:\n\n");
		}//endfor
		
		//UDP connection teardown
		if(sendinfo->buffer.proto == PROTO_UDP) {
		 	msg_iov->iov_len =  ah.mysize;//sizeof(struct udp_addhdr);
		 	ah.seq = 1;
		 	msg_iov->iov_base = &ah;
		 	sendmsg(fd,msgh, 0  );
		}

}


void init_sendinfo(struct s_sendinfo *sendinfo) {
	sendinfo->fd = (int*) cmalloc(sizeof(int));
	sendinfo->buffer.begin = (char*) cmalloc(sizeof(char) * 65536);
	sendinfo->buffer.allocated_space = 65536;
	sendinfo->buffer.buf_size = 0;
	sendinfo->buffer.buf = sendinfo->buffer.begin + sendinfo->buffer.allocated_space;
	
//	sendinfo->buffer.type = NOT_DEFINED;	
	sendinfo->msg.msg_control = NULL;
	sendinfo->msg.msg_controllen = 0;
//	sendinfo->udp_length = NULL;	
}


void init_generator(struct s_generator *gen, struct s_config *conf){
	if(conf->kernel_packet == TRUE) {
		if(conf->network == TYPE_IP4 && conf->transport == TYPE_TCP) {
			gen->set_nethdr = set_ipsockopts;
			gen->set_transhdr = set_tcpsockopts;
			gen->init_socket = init_tcpsocket;
		}
		if(conf->network == TYPE_IP6 && conf->transport == TYPE_TCP) {
			gen->set_nethdr = set_ipsockopts;
			gen->set_transhdr = set_tcpsockopts;
			gen->init_socket = init_tcpip6socket;
		}
		// TODO socket options!
		if(conf->network == TYPE_IP4 && conf->transport == TYPE_UDP) {
			gen->set_nethdr = set_ipsockopts;
			gen->set_transhdr = set_udpsockopts;
			gen->init_socket = init_udpsocket;
		}
		if(conf->network == TYPE_IP6 && conf->transport == TYPE_UDP) {
			gen->set_nethdr = set_ipsockopts;
			gen->set_transhdr = set_udpsockopts;
			gen->init_socket = init_udpip6socket;
		}	
	}

	
	if(conf->kernel_packet == FALSE) {
		if(conf->network == TYPE_IP4 && conf->transport == TYPE_TCP) {
			gen->set_nethdr = fill_ip4hdr;
			gen->set_transhdr = fill_tcphdr;
			gen->init_socket = init_rawsocket;
		}
		if(conf->network == TYPE_IP6 && conf->transport == TYPE_TCP) {
			gen->set_nethdr = fill_ip6hdr;
			gen->set_transhdr = fill_tcphdr;
			gen->init_socket = init_rawsocket6;
		}
		if(conf->network == TYPE_IP4 && conf->transport == TYPE_UDP) {
			gen->set_nethdr = fill_ip4hdr;
			gen->set_transhdr = fill_udphdr;
			gen->init_socket = init_rawsocket;
		}
		if(conf->network == TYPE_IP6 && conf->transport == TYPE_UDP) {
			gen->set_nethdr = fill_ip6hdr;
			gen->set_transhdr = fill_udphdr;
			gen->init_socket = init_rawsocket;
		}
		if(conf->network == TYPE_IP4 && conf->transport == TYPE_ICMP) {
			gen->set_nethdr = fill_ip4hdr;
			gen->set_transhdr = fill_icmphdr;
			gen->init_socket = init_rawsocket;
		}
		if(conf->network == TYPE_IP6 && conf->transport == TYPE_ICMP6) {
			gen->set_nethdr = fill_ip6hdr;
			gen->set_transhdr = fill_icmp6hdr;
			gen->init_socket = init_rawsocket6;
		}
		
	}
}



void check(char *msg, int c) {
	if(c < 0) {
		perror(msg);
		exit(0);
	}
}
void check_warning(char *msg, int c) {
	if(c < 0) {
		perror(msg);
	}
}



//*****************************************
// Timer functions *
//*****************************************
void tvadd(tsum, t0, t1)
	struct timeval *tsum, *t0, *t1;
{

	tsum->tv_sec = t0->tv_sec + t1->tv_sec;
	tsum->tv_usec = t0->tv_usec + t1->tv_usec;
	if (tsum->tv_usec > 1000000)
		tsum->tv_sec++, tsum->tv_usec -= 1000000;
}



void tvsub(tdiff, t1, t0)
	struct timeval *tdiff, *t1, *t0;
{

	tdiff->tv_sec = t1->tv_sec - t0->tv_sec;
	tdiff->tv_usec = t1->tv_usec - t0->tv_usec;
	if (tdiff->tv_usec < 0)
		tdiff->tv_sec--, tdiff->tv_usec += 1000000;
}




void sectotv(double sec, struct timeval *tv) {
	long floor = (long)sec;
	tv->tv_sec = floor; 	
	tv->tv_usec = (sec - floor) * 100 * 1000;
	//fprintf(stderr, "\t-->converting to  %i secs, %i msecs\n", tv->tv_sec, tv->tv_usec);
}




void wait_timeval(struct timeval *tv) {
  // fprintf(stderr, "\t-->waiting %ld seconds, %ld mic seconds\n", tv->tv_sec, tv->tv_usec);
  select(0, (fd_set *)0, (fd_set *)0, (fd_set *)0, (tv));
}


void set_nethdr_NOT_DEFINED() {
	fprintf(stderr, "Internal error (s_generator): function set_nethdr not defined\n");
	exit(0);
}
void set_transhdr_NOT_DEFINED() {
	fprintf(stderr, "Internal error (s_generator): function set_trans not defined\n");
	exit(0);
}
void init_socket_NOT_DEFINED() {
	fprintf(stderr, "Internal error (s_generator): function init_socket not defined\n");
	exit(0);
}


