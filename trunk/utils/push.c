/*
 * Test Program to send and receive UDP packets over an
 * IP link. May be used for measuring bandwidth of an IP link.
 *
 * Hacked by Toerless.Eckert@informatik.uni-erlangen.de 1991..95
 *
 * Changes by Falko.Dressler@rrze.uni-erlangen.de 1997..98
 */

#undef HASTE
#undef USE_SENDTO

#undef HRTIME

#ifdef __SVR4
#ifndef SYSV
#define SYSV
#endif
#endif

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#ifndef HP
#include <arpa/inet.h>
#endif
#include <netdb.h>

#ifdef SYSV
#define bcopy(FROM,TO,LEN) memcpy(TO,FROM,LEN)
#include <string.h>
#define rindex strrchr
#else
#include <strings.h>
#endif

#include <sys/errno.h>
#include <signal.h>
#include <sys/time.h>

#ifdef IP_ADD_MEMBERSHIP
#define MULTICAST
#endif

extern char *optarg;
extern int optind, opterr;
/* extern unsigned long inet_addr(); */
extern int errno;

/*
 * global constants
 */

#define PORT 7000                 /* default port to use to send udp packets */
#define MAXPSIZE (24 * 1024)       /* maximum packet size to send             */
#define MINPSIZE 64               /* minimum packet size to send             */
#define IPHDRLEN (20 + 8)	  /* length in bytes of IP + UDP header      */
#define TRATE    10000000         /* transmission rate of first network      */
				  /* in bps, ethernet assumed                */

/*
 * routines of this program
 */

static void usage();
static void do_send();
static void do_receive();
static void getalarm();
static void myusleep();

static char *progname;            /* name of this program                    */
static int mint;		  /* measuring interval                      */
static int now;   		  /* time of here and now                    */
static int socksize = 24 * 1024;  /* extended buffer size to use, sun-tuned  */
static int sockopt;
static int delay;
static int num;
static int kbytes;
static int variable;
static int hsize;
static int quiet;
static int pdebug;
static int fill;
static int source;
#ifdef MULTICAST
static char ttl = 1;
#endif
static char *name = "";

/*
 * Array of packet length distribution for
 * variable length packet mix tests
 * This array should not be harddwired
 */

struct spdist { int len; int count; };

static struct spdist pdist[] = {
       64,  14000,
       1500, 14000 };


main(argc,argv)
int argc;
char *argv[];
{
    char *cp, *ap;
    int c;
    int low, high, tout, send, receive; 
    struct hostent *hp;
    struct sockaddr_in sa;
#ifndef HP
    struct itimerval timer;
    struct sigaction action;
#endif
    int port,i;

    /** set defaults for parameters
    **/

    port = PORT;
    low = MINPSIZE;
    high = MAXPSIZE;
    tout = -1;
    send = 0;
    num = 0;
    receive = 0;
    kbytes = 0;
    variable = 0;
    sockopt = 0;
    delay = 0;
    hsize = 0;
    mint = 1;
    quiet = 0;
    pdebug = 0;
    fill = 0;
    source = 0;

    progname = (cp = rindex(argv[0],'/')) ? cp + 1 : argv[0];

#ifdef MULTICAST
    while ((c = getopt(argc, argv, "Dd:f:H:h:k:L:l:m:n:op:qr:s:S:T:t:v")) != -1) {
#else
    while ((c = getopt(argc, argv, "Dd:f:H:h:k:L:l:m:n:op:qr:s:S:t:v")) != -1) {
#endif
	switch(c) {

        /** -D     - Print out packets received (length)
	 **          a single packet between sending the packets.
	 **/

	case 'D':
	    pdebug = 1;
#ifndef HRTIME
	    fprintf(stderr, "gethrtime(3) not available, -p not implemented\n");
#endif
	    break;

        /** -d <n> - Sleep myusleep <n>% of the time needed to send
	 **          a single packet between sending the packets.
	 **/

	case 'd':
	    delay = atoi(optarg);
	    break;

        /** -f     - Fill packets (on sending) with random date.
	 **          Default are 0-filled packets.
	 **/

	case 'f':
	    fill = atoi(optarg);
	    break;


	/** -H <bytes>  - Size of additional header to calculate with.
	 **/

	case 'H':
	    hsize = atoi(optarg);
	    break;

        /** -h <n> - Maximum size of packets to be used.
	 **/

	case 'h':
	    high = atoi(optarg);
	    if( high < MINPSIZE || high > MAXPSIZE) {
		fprintf(stderr, "-h argument out of range: %d < size < %d\n",
		              MINPSIZE, MAXPSIZE);
		exit(1);
	    }
	    break;

        /** -k <kbyte> - Data rate to send in kbyte/sec.
	 **/

	case 'k':
	    kbytes = atoi(optarg);
	    break;

        /** -l <byte> - Lowest size of packets to be used.
	 **/

	case 'l':
	    low = atoi(optarg);
	    if( low < MINPSIZE || low > MAXPSIZE) {
		fprintf(stderr, "-h argument out of range: %d < size < %d\n",
		              MINPSIZE, MAXPSIZE);
		exit(1);
	    }
	    break;

        /** -L <label> - Label to output on every line
	 **/

	case 'L':
	    { int l;
	      l = strlen(optarg);
	      name = malloc(strlen(optarg)+20);
	      sprintf(name, "%s:%*s", optarg, 14 - l, " ");
	    }
	      break;


	/** -m <seconds> - Measuring interval. Print out measurements every 
	 **          <n> seconds. Te higher <n> the more exact should the
	 **          outcome be.
	 **/

	case 'm':
	    mint = atoi(optarg);
	    break;

	/** -n <packets/sec> - Data rate to send in packets per second.
	 **/

	case 'n':
	    num = atoi(optarg);
	    break;

	/** -o     - Use larger UDP buffers.
	 **/

	case 'o':
	    sockopt++;
	    break;

	/** -p <port-number>  - Use a different port for the data transfer
	 **/

	case 'p':
	    port = atoi(optarg);
	    break;

	/** -q - on the receiver side write only notifications if data comes in
	 **/

	case 'q':
	    quiet = 1;
	    break;

	/** -v     - Send or receive variable length packets.
	 **/

	case 'v':
	    variable = 1;
	    break;
	
	/** -r <ip-addr> - Select receive mode of operation.
	 **/

	case 'r':
	    receive++;
	    ap = optarg;
	    break;

        /** -S     - Source packets from stdin (on send)
	 **          or to stdout (on receive)
	 **/

	case 'S':
	    source = 1;
	    break;

	/** -s <ip-addr> - Select send mode of operation.
	 **/

	case 's':
	    send++;
	    ap = optarg;
	    break;

	/** -t <seconds> - Length of time to send packets each length.
	 **/

	case 't':
	    tout = atoi(optarg);
	    break;

#ifdef MULTICAST
	case 'T':
	    ttl = atoi(optarg);
	    break;
#endif

	default:
	    usage();
	    exit(1);
	}
    }

    /** Check options for consistency
     **/
    
    if( receive && send || !receive && !send) {
	usage();
	exit(1);
    }
    if( send && ((i = (kbytes||0) + (delay||0) + (num||0)) > 1)) {
	fprintf(stderr, "%s: -d and -k option cannot be used together.\n",
			progname);
	exit(1);
    } else if(send && (i == 0)) {
	fprintf(stderr, "%swarning: sending data at maximum rate.\n", name);
    }

    /** Resolve hostname of remote site
     **/
    
    if ((hp = gethostbyname(ap)) == NULL) {
	if((sa.sin_addr.s_addr = inet_addr(ap)) == -1) {
	    fprintf(stderr, "%s%s: illegal ip address %s\n", name, progname, ap);
	     exit(1);
	}
	sa.sin_family = AF_INET;
    } else {
        bcopy((char *)hp->h_addr, (char *) &sa.sin_addr, hp->h_length);
	sa.sin_family = hp->h_addrtype;
    }
    sa.sin_port = htons(port);

    /** start clock for alarms
     **/

#ifdef HP
    signal(SIGALRM, getalarm);
#else
    (void) sigaction(SIGALRM, (struct sigaction *) NULL, &action);
    action.sa_handler = getalarm;
    if (receive) {
	/* (SA_INTERRUPT on 4.1.x) = !(SA_RESTART on 5.x) */
#ifdef SA_RESTART
        /* compiled with SunOS 5.x */
	action.sa_flags &= ~SA_RESTART;
#else
        /* Compiled with SunOS 4.1.x */
	if(action.sa_flags & SA_INTERRUPT) {
	    fprintf(stderr, "SunOS 4.1.x compatibility library broken. Forget it.\n");
	    exit(-1);
	} else {
	    action.sa_flags |=  SA_INTERRUPT;
	    action.sa_flags &= ~SA_RESETHAND;
	}
#endif
    }
    (void) sigaction(SIGALRM, &action, (struct sigaction *) NULL);
#endif

    now = 0;

#ifdef HP
    if(  alarm(mint) < 0) {
        perror("alarm");
	exit(errno);
    }
#else
#ifdef HASTE
    timer.it_interval.tv_sec = mint / HASTE;
    timer.it_interval.tv_usec = (mint * 1000000 / HASTE) % 1000000;
#else
    timer.it_interval.tv_sec = mint;
    timer.it_interval.tv_usec = 0;
#endif
    timer.it_value = timer.it_interval;
    if( setitimer(ITIMER_REAL, &timer, (struct itimerval *) 0) == -1) {
	perror("setitimer");
	exit(errno);
    }
#endif

     hsize += IPHDRLEN;

    if (receive) {
	do_receive(low, high, tout, sa);
    } else {
	do_send(low, high, tout, sa);
    }

}

static void do_receive(minp, maxp, t, addr)
int minp, maxp, t;
struct sockaddr_in addr;
{
    int s, n;
    struct sockaddr_in sa;
    char buf[MAXPSIZE];
    int lately, i, b;
    int psize;
    double throughput;
    int true = 1;
#ifdef HRTIME
    hrtime_t hrlast, hrdelta, hrnow;
    hrlast = -1;
#endif


    /** Get socket for operation. We use a +1 offset for the sender port
     ** so that we can use a sender and receiver on the same local host
     ** without needing to worry.
     **/
     
    if((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
	perror("socket(receive)");
	exit(errno);
    }

    if(sockopt) {
        if (setsockopt(s, SOL_SOCKET, SO_SNDBUF, (char *)&socksize,
                                sizeof (socksize)) < 0) {
            perror("set send buffer (ignored)");
        }
        if (setsockopt(s, SOL_SOCKET, SO_RCVBUF, (char *)&socksize,
                                    sizeof (socksize)) < 0) {
            perror("set receive buffer (ignored)");
        }
    }

#ifdef MULTICAST
    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *) &true,
			   sizeof(true)) == -1) {
        perror("so_reuseaddr");
        exit(errno);
    }
#endif


    sa.sin_family      = AF_INET;
    sa.sin_addr.s_addr = INADDR_ANY;
    sa.sin_port        = addr.sin_port;
    if( bind(s, (struct sockaddr *) &sa , sizeof(sa)) == -1) {
        perror("bind(receive)");
        exit(errno);
    }

#ifdef MULTICAST
    if(IN_MULTICAST(ntohl(addr.sin_addr.s_addr))) {
        int false = 0;
        struct ip_mreq req;

	req.imr_multiaddr = addr.sin_addr;
	req.imr_interface.s_addr = INADDR_ANY;

        if (setsockopt(s, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *) &req,
			   sizeof(req)) == -1) {
            perror("ip_add_membership");
            exit(errno);
	}
    } else
#endif
    {
	/* unicast case: 
	 * connect selects to receive only packets with originiator
	 * address of addr and addr.sin_port + 1
	 */

        addr.sin_port = htons(ntohs(addr.sin_port) + 1);
        if( connect(s, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
            perror("connect(receive)");
            exit(errno);
        }
    }

    i = 0;
    b = 0;
    psize = 0;
    lately = now;
    if(!num) {
	num = -1;
    }
    printf("%sReader starting %s\n", name, quiet ? "(quiet)" : "" );
    fflush(stdout);
    while( n = read(s, buf, sizeof(buf))) {
	if(n == -1) {
	    if(errno != EINTR) {
	        perror("read");
	    } else {
		n = 0; 
		goto presult;
	    }
	}
	else {
	    n += hsize;

#ifdef HRTIME
	    if(pdebug) {
		if(hrlast < 0) {
		    /* first time */
		    hrdelta = 0;
		    hrlast = gethrtime();
		} else {
                    hrnow   = gethrtime();
		    hrdelta  = hrnow - hrlast;
		    hrlast  = hrnow;
		}
		printf("  %6.05f LEN= %d\n", ((double) hrdelta) / 1000000000.0 , n - 20);
	    }
#endif

	    b += n;
	    if(!variable && (n != psize)) {
		printf("%%%d", n);
		fflush(stdout);
		psize = n;
		i = 1;
		b = n;
	    } else {
	        i++;
		if(now > lately) {
		presult:
		    throughput = (double) b / (double) (now -lately) / 1024.0;
		    if(!pdebug && (!quiet || i)) {
		        printf(
		        "\n%sReceived %4d pps of %s %4d in %3d sec = %9.3f kbyte/sec",
			    name,
		            i / (now - lately),
			    variable ? "mean length" : "size",
			    variable ? (b / (i + 1)) : psize,
			    now - lately,
			    throughput
		        );
		    }
		    fflush(stdout);
		    i = 0;
		    b = 0;
		    lately = now;
		}
	    }
        }	
	if(n >= 0) {
	    if(! --num) {
	       break;
	    }
	}
    }
    printf("%s\ndone\n",name);

}

static void do_send(minp, maxp, t, addr)
int minp, maxp, t;
struct sockaddr_in addr;
{
    int s;
    struct sockaddr_in sa;
    char buffer[MAXPSIZE*3];
    char *buf; int bufsize;
    int psize, size;
    int n, i, endtime, lately, dsleep, tsleep;
    double rate = 0;
    int dtime, stime, b, b2;
    int pn, k, k2, deltak, pdistn;
    double throughput;
    struct timeval tstart;
    int true=1;

    if(fill) {
	get_time(&tstart);
	srand(tstart.tv_sec);
	for(i=0; i < sizeof buffer; i++) {
	    if(fill > 255) {
		buffer[i] = (char) (rand() & 0xff);
	    } else {
	        buffer[i] = (char) fill;
	    }
	}
    } else {
	memset(buffer, 0, sizeof buffer);
    }
    /* Make buf odd byte aligned */

    buf = (char *) ( ((int) buffer & ( ~ (4096 -1))) + 4096);
    /* fprintf (stderr, "&buffer = 0x%x, buf = 0x%x\n", buffer, buf); */
    bufsize = MAXPSIZE;


    /** Get socket for operation. We use a +1 offset for the sender port
     ** so that we can use a sender and receiver on the same local host
     ** without needing to worry.
     **/


    if((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
	perror("socket");
	exit(errno);
    }

#ifdef SO_REUSEADDR
    if(IN_MULTICAST(ntohl(addr.sin_addr.s_addr))) {
        if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *) &true,
			   sizeof(true)) == -1) {
            perror("so_reuseaddr");
            exit(errno);
        }
    }
#endif
    if(sockopt) {
        if (setsockopt(s, SOL_SOCKET, SO_SNDBUF, (char *)&socksize,
                                sizeof (socksize)) < 0) {
            perror("set send buffer (ignored)");
        }
        if (setsockopt(s, SOL_SOCKET, SO_RCVBUF, (char *)&socksize,
                                    sizeof (socksize)) < 0) {
            perror("set receive buffer (ignored)");
        }
    }

    sa.sin_addr.s_addr = INADDR_ANY;
    sa.sin_port = htons( ntohs(addr.sin_port) + 1);

    if( bind(s, (struct sockaddr *) &sa, sizeof(sa)) == -1) {
	perror("bind");
	exit(errno);
    }

#ifndef USE_SENDTO
    if( connect(s, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
	perror("connect");
	exit(errno);
    }
#endif

#ifdef MULTICAST
    if(IN_MULTICAST(ntohl(addr.sin_addr.s_addr))) {
        struct ip_mreq req;
	char false = 0;

        if (setsockopt(s, IPPROTO_IP, IP_MULTICAST_TTL, &ttl,1)==-1) {
            perror("ip_multicast_ttl");
            exit(errno);
        }
        if ( setsockopt(s, IPPROTO_IP, IP_MULTICAST_LOOP, &false,1) == -1) {
            perror("ip_multicast_loop");
            exit(errno);
        }
    }
#endif

    if(variable) {
	srand48();
	pn = sizeof(pdist) / sizeof(struct spdist);
	pdistn = 0;
        for(i=0; i < pn; i++) {
            pdistn += pdist[i].count;
	}
	printf("pdistn = %d\n", pdistn);
    }

    psize = minp;
    printf("Writer starting\n");

    while (psize <= maxp) {
	size = psize - hsize;
	tsleep = 0;
	if (variable) {
	    printf("Writing variable length packets\n");
	}
	else {
	   printf("Writing packets of length %d (data) + %d (header) = %d\n",
	           size, hsize, size + hsize);
	}
	endtime = now + t;
	lately = now;
        dtime = delay * (size + hsize ) * 8 * 1000000 / TRATE;
	stime = 0;
        get_time(&tstart);
	i = 0;
	k = 0;
	k2 = 0;
	deltak = 1;
	b = 0;
	b2 = 0;
	if(kbytes) {
	    num = kbytes * 1024 / psize;
	} else {
	    kbytes = num / 1024 * psize;
	}
	while((t < 0) || (now < endtime)) {
            /* fprintf(stderr, "now = %d\n", now); */
	    if(variable) {
		extern double drand48();
		int r = (int) (drand48() * (double) pdistn);
		int r1 = r;
		int index = 0;

		while( r > pdist[index].count ) {
		    r -= pdist[index].count;
		    index ++;
		}
		size = pdist[index].len - hsize;
		/* printf("%d, %d, %d, %d\n", pdistn, r1, index, size); */
	    }
#ifdef USE_SENDTO
            if((n =  sendto(s, buf, size, 0,
			   (struct sockaddr *) &addr, sizeof(addr))) != size) {
#else
	    if((n = write(s, buf, size)) != size) {
#endif
		switch(errno) {
		case ECONNREFUSED: 
		    printf("%%");
		    fflush(stdout);
		    break;
		case EHOSTUNREACH:
		    printf("*");
		    fflush(stdout);
		    break;
		case ENETUNREACH:
		    printf("$");
		    fflush(stdout);
		    break;
		case ENETDOWN:
		    printf("@");
		    fflush(stdout);
		    break;
		case ENOBUFS:
		    printf("&");
		    fflush(stdout);
		    myusleep(40000);
		    break;
		case EINTR:
		    break;
		default:
		    perror("write");
		    break;
		}
	    } else {
		i++;                 /* increase packet sent counter */
		b += size + hsize;   /* increase bytes sent counter  */
		b2 += size + hsize;  /* increase total bytes sent counter */
		if(num)
		  if( k++ >= k2) {
		    /* delay so that we send packets with a constant rate */
		    struct timeval tnow;
		    int tsleep;
		    double usecs_should_spent;
		    double usecs_have_spent;

		    get_time(&tnow);

		    usecs_should_spent = (tnow.tv_sec - tstart.tv_sec)
					 * 1000000.0
	                                 + tnow.tv_usec - tstart.tv_usec;

		    if(variable) {
	                usecs_have_spent = (((double) b)/((double) kbytes) /
					   1024.0) *1000000.0;
		    } else {
	                usecs_have_spent = (((double) k) / ((double) num)) *
					   1000000.0;
		    }

	            tsleep =usecs_have_spent - usecs_should_spent;

		    if(tsleep > 0) {
			/*
			 *we are sending too fast,
			 * sleep some time
			 */
			myusleep(tsleep);
			deltak--;
			/* fprintf(stderr, "%%%d-%d",deltak, tsleep); */
		    } else {
			/* 
			 * we are still too slow, must not do the systemcall
			 * get_time each time we send, slows us down.
			 * calculate how many packets we should send at
			 * max speed before we'll have to check tim again
			 */
			 deltak++;
			/* fprintf(stderr, ".%d",deltak);  */
		    }
		    k2 = k + deltak;
		  }
	        if(dtime) {
		    stime += dtime;
		    if(stime > 0) {
		        struct timeval tp1, tp2;
			int dusec, dsec;

		        get_time(&tp1);
			myusleep(stime);
		        get_time(&tp2);

			stime -= (tp2.tv_sec - tp1.tv_sec) * 1000000
			         + tp2.tv_usec - tp1.tv_usec;
		    }
	        }
		if( now > lately) {
		    throughput = (double) (b2) /
				 (double) (now -lately) / 1024.0;
		    if(variable) {
		        printf("\n%sSent %4d pps of mean length %4d in %3d sec = %9.3f kbytes/sec",
		        name, i / (now - lately), b2 / i,now - lately, throughput);
		    }
		    else {

		        printf("\n%sSent %4d pps of size %4d in %3d sec = %9.3f kbytes/sec",
		        name, i / (now - lately), psize, now - lately, throughput);
		    }
		    fflush(stdout);
		    lately = now;
		    i = 0;
		    b2 = 0;
		}
	    }
	}
	printf("\ndone\n");
	fflush(stdout);
	sleep(10);

	if ((psize * 2 >  maxp) && (psize < maxp) ) {
		psize = maxp;
	} else {
		psize *= 2;
	}
    }
    for(i = 1; i < 10; i++) {
	if((n = send(s, buf, 0,0)) != 0) {
	    perror("send(0)");
	}
    }
}

static void usage()
{
    fprintf(stderr,"usage: %s { options...}\n", progname);
    fprintf(stderr,"-D               - (on receive) print size of packets and spacing between them\n");
    fprintf(stderr,"-d <part>        - delay between packets for <part> of packet\n");
    fprintf(stderr,"-f               - (on send) fill packets with random data. 0 filled otherwise\n");
    fprintf(stderr,"-H <bytes>       - calculate with additional header size (default IP + UDP)\n");
    fprintf(stderr,"-h <bytes>       - high packet size (default %d bytes)\n",MAXPSIZE);
    fprintf(stderr,"-k <kbytes>      - send with data rate <kbyte/sec\n");
    fprintf(stderr,"-L <label>       - label to prefix every output line with\n", MINPSIZE);
    fprintf(stderr,"-l <bytes>       - low packet size (default %d bytes)\n", MINPSIZE);
    fprintf(stderr,"-m <seconds>     - write results every <seconds> (default %d seconds)\n", 1);
    fprintf(stderr,"-n <n>           - send <n> packets/sec\n");
    fprintf(stderr,"                 - by default the sender sends as fast as he can\n");
    fprintf(stderr,"-o               - use large (%d kbyte) udp buffers\n", socksize);
    fprintf(stderr,"-p <port>        - send to port <port> (default %d)\n", PORT);
    fprintf(stderr,"-q               - be quiet on receivers side if nothing is received");
    fprintf(stderr,"-r <ip-address>  - receive data from ip-address\n");
    fprintf(stderr,"-s <ip-address>  - send data to ip-address\n");
    fprintf(stderr,"-t <seconds>     - duration to send packets of each size (default is continuous)\n");
#ifdef MULTICAST
    fprintf(stderr,"-T <ttl>         - time to live for multicast packets (default is 1)\n");
#endif
    fprintf(stderr,"-v               - send/receive variable length packet mix\n");
    fprintf(stderr,"\n");
    fprintf(stderr,"%s sends or receives streams of UDP packets for measuring\n", progname);
    fprintf(stderr,"the throughput of the IP link between the hosts.\n");
    fprintf(stderr,"%s will send packets of different length consecutively.\n", progname);
    fprintf(stderr,"It will start with the low packet size and double that size until\n");
    fprintf(stderr,"the packet size becomes larger than the high packet size.\n");
}

static void getalarm(sig)
int sig;
{
    /* simply increment a global variable to hold the time */
    now += mint;

#ifdef HP
    signal(SIGALRM, getalarm);
    if(  alarm(mint) < 0) {
        perror("alarm");
	exit(errno);
    }
#endif
}

/** a usleep that does not need the  ITIMER_REAL
 **/

static void myusleep(t)
int t;
{
    struct timeval tsleep;

#ifdef HASTE
    t /= HASTE;
#endif

    tsleep.tv_sec = t / 1000000;
    tsleep.tv_usec = t % 1000000;
    (void) select(0, NULL, NULL, NULL, &tsleep);
}

static get_time(t)
struct timeval *t;
{
#ifdef HASTE
    static struct timeval t0 = { -1, -1};
#endif

    gettimeofday(t, 0);

#ifdef HASTE
    if(t0.tv_sec == -1) {
	t0 = *t;
    } else {
        t->tv_usec = t0.tv_usec + (t->tv_usec - t0.tv_usec) * HASTE;
        t->tv_sec  = t0.tv_sec  + (t->tv_sec  - t0.tv_sec)  * HASTE;
	while(t->tv_usec > 1000000) {
	    t->tv_usec -= 1000000;
	    t->tv_sec++;
	}
	while(t->tv_usec < 0) {
	    t->tv_usec += 1000000;
	    t->tv_sec--;
	}
    }
#endif
}
