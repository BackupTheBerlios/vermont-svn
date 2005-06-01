
#include <stdio.h>
#include <pthread.h>
#include <string.h> /* strcpy, memset */
#include <stdlib.h> /* malloc */
#include <unistd.h> //get_pid

#include "memory.h"
#include "config.h"


/* maximum length of configuration file name */
//#define MAXFILENAME	20

/* some prototype definitions */
void wait_for_threads		__P(());
void* exec_thread			__P((void *traffic));
void start_automata			__P((struct s_config *conf, FILE *fp));
static void usage			__P(());

/* condition variable */
pthread_cond_t master_wait = PTHREAD_COND_INITIALIZER;
pthread_mutex_t master_mutex = PTHREAD_MUTEX_INITIALIZER;


#define TRUE 1
#define FALSE 0

int verbose = 0;
int num_of_threads = 0;
FILE *output;
int out = 0;

int main(int argc, char**argv) {

	pthread_t p_thread;
	struct s_config *conf;
	char filename[MAXFILENAME];
	char output_file[MAXFILENAME];
	FILE *fp;

	/* read input parameters */
	if(argc < 3) {
		usage();	
	}
	argv++; argc--;
	while( argc > 0 && argv[0][0] == '-' )  {
		switch (argv[0][1]) {
			case 'f': /* next string specifies a filename */
				strncpy(filename,&argv[1][0] , MAXFILENAME);
				argv++; argc--; /* skip filename */
				break;
			case 'v': 
				verbose = 1; 
				if(argv[0][2] == 'v')
					verbose = 2;
				break;
			case 'o': /* next string specifies a filename */
				out = 1;
				strncpy(output_file,&argv[1][0] , MAXFILENAME);
				argv++; argc--; /* skip output filename */
				break;
			default:
				usage();
		};
		argv++; argc--;
	}
	
	
	if(out) {
		output = fopen(output_file, "a");
		if(!output) {
			fprintf(stderr, "Failed opening file for output %s: ", output_file);
			perror("");	
		}	
	}
	
	
	/* open script file for reading */
	fp = fopen(filename, "r");
	if (!fp) {
		fprintf(stderr, "Failed opening file %s: ", filename);
		perror("");
		exit(1);	
	}	

	/* read the script file */
	//if(verbose)
	fprintf(stdout, "Reading configuration file...\n");
	
	conf = (struct s_config*) cmalloc(sizeof(struct s_config));
	
	/* the automata reads the script language from the
	 * file and fills the conf structure  */
	start_automata(conf, fp);
	
	//if(verbose)
		fprintf(stdout, "File is OK\n\n");

	/* close script file */
	if (fclose(fp)) {
    		perror("Failed closing file ':");
    		//exit(1);
	}

	fprintf(stdout, "Starting threads...\n\n");
	/* every conf specifies a single stream. The next stream
	 * is specified in conf->next. For every stream a thread is
	 * created */
	while(conf != NULL) {
		num_of_threads++;
		pthread_create(&p_thread, NULL, exec_thread, conf);
		conf = conf->next;		
	}

	/* main waits for the threads to finish their work */
	wait_for_threads();
	fprintf(stdout, "\nThere is no more work to do. Bye\n");

	return 0;
}



void wait_for_threads() {
	pthread_mutex_lock(&master_mutex);
	pthread_cond_wait(&master_wait, &master_mutex);
	pthread_mutex_unlock(&master_mutex);
}



static void usage() {
	printf("Usage: netpag -f [filename] [-options]\n\n" \
			"Options: \n"\
			"\t-v verbose output\n" \
			"\t-vv even more outout\n"\
			"\t-o [filename] specifies an output filename. Raw packets are\n" \
			"\t              printed into the file in HEX format\n"
	 );
	exit(0);	
}









/* Reads 'size' characters from file 'filename' (starting at position 'pos')
   into buffer 'buf'.
   Return:  The number of characters written to the buffer.

   If 'filename' is empty, 'size' random characters are generated.
 */
int set_payload(char *buf, int size, char *filename, int format, int pos) {
	int cwrite = 0; /* number of characters written to buffer until now */
	int isRand = 0; // TODO this should be boolean

	
	if(strcmp(filename, "") == 0) isRand = 1;
	else isRand = 0;
	
	//fprintf(stderr, "filename = %s, rand = %i\n", filename, isRand);
	
	if(!isRand) {
		FILE *fp;
		fp = fopen(filename, "r");
		if (!fp) {
			fprintf(stderr, "%i: ** WARNING ** : Failed opening file '%s' to read payload. "\
			                "Generating a random payload! \n" ,getpid(), filename);
			isRand = 1;
		}
		else {
			/* opening file succeeded */
			int c;

			/* goto position pos (skip pos characters) */
			while(pos != 0) {
				c = fgetc(fp);
				if(c == EOF) {
					ungetc(c, fp);
					break;
				}
				pos--;
			}
						
			if(format == HEX) {
				//fprintf(stderr, "IS_HEX...\n");
				while(cwrite < size) {
					int val;
					char hex[2];
					fgets(hex, 3, fp);
					//fprintf(stderr, "hex = %s\n", hex);
					val = strtol(hex, (char**)NULL, 16);
				
					*(buf + cwrite) = (char)val;
					//skip blank
					fgetc(fp);
					cwrite++;
				}

			}
			else {
				/* read characters into buffer */
				while(cwrite < size) {
					c = fgetc(fp);
					if(c == EOF) break;
					*(buf + cwrite) = c;
					cwrite++;
				}
			}


		}
	}

	/* No filename specified. Generate a random payload */
	if(isRand) {
		int i;
		for(i = 0; i < size; ++i) {
			*(buf + cwrite) = (rand() % 70) + 50;
			cwrite++;
		}
	}

	if(cwrite < size) {
		fprintf(stderr, "%i: ** WARNING ** : The payload size was specified to be %i bytes but the file contains only" \
		" %i bytes\n", getpid(), size, cwrite);
	}
	return cwrite;
}

















