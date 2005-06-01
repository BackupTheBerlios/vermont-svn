
#include <stdio.h>
#include <string.h>
#include <stdlib.h> //atof
#include "memory.h"

#include "config.h"
#include "automata.h"
#include "states.h"

//TODO if you change conf->ipproto you may not need this anymore ?!
#include <netinet/in.h> //IPPROTO_XXX

/*
#ifdef __P
#undef __P
#endif
#define __P(prots)    prots
*/

/* global variables */

int fp_line = 1; /* line of FILE pointer */
int word_line;	/* line of word that was read last */

struct addrinfo **tmpip_src;
struct addrinfo **tmpip_dst;
int *tmpipproto;



/********************************************************************************
 * Deterministic finit automata. A word is read from the input file. The automata
 * is then fed with the word.
 ********************************************************************************/
void start_automata(struct s_config *conf, FILE *fp) {

	char word[MAXWORDSIZE], val[MAXWORDSIZE];
	struct s_automata automata;


	reset_automata(&automata);
	init_automata();

	conf->traffic.pattern = NULL;
	
	tmpip_src = (struct addrinfo**)malloc(sizeof(struct addrinfo*));
	tmpip_dst = (struct addrinfo**)malloc(sizeof(struct addrinfo*));
	tmpipproto = (int*) malloc(sizeof(int));


	while(get_next_word(word, fp) != EOF) {
		/* remember line of word */
		word_line = fp_line; 

		/* if the automata is reseted a new stream definition should follow */
		if(automata.reset == TRUE) {
			conf->next = (struct s_config*)malloc(sizeof(struct s_config));
			conf = conf->next;
			conf->traffic.pattern = NULL;
			
			tmpip_src = (struct addrinfo**)malloc(sizeof(struct addrinfo*));
			tmpip_dst = (struct addrinfo**)malloc(sizeof(struct addrinfo*));
			tmpipproto = (int*) malloc(sizeof(int));

			reset_automata(&automata);
		}

		int next_state = get_next_state(automata.state, word);	
		if(next_state != -1) {
			struct s_node *node;
			automata.state = next_state;
			node = get_node(automata.state);
			if(node->init != NULL) node->init(conf);	
		}
		else {
			struct s_node *node;
			node = get_node(automata.state);
			if(node != NULL) {
				get_pair_value(val, fp);	
				node->read(word, val, conf, &automata);
			}
			else {
				fprintf(stderr, "INTERNAL ERROR: automata in unknown state (%i)\n", automata.state);
				exit(0);
			}	
			
		}
	}
	if(automata.state != BEGIN) {
		fprintf(stderr, "ERROR: unexpected end of file, %i\n", automata.state);
		exit(0);	
	}
	conf->next = NULL;

}

void init_automata() {
		
	/* behavior of the automata */
	add_state(BEGIN, NULL, begin, NULL);
	add_state(IP, init_ip, read_ip, check_ip);	
	add_state(TCP, init_tcp, read_tcp, check_tcp);
	add_state(UDP, init_udp, read_udp, check_udp);
	add_state(ICMP, init_icmp, read_icmp, check_icmp);
	add_state(ICMP6, init_icmp6, read_icmp6, check_icmp6);
	add_state(IP6, init_ip6, read_ip6, check_ip6);	
	
	add_state(STREAM, init_stream, read_stream, check_stream);
	add_state(TRAFFIC, init_traffic, read_traffic, check_traffic);
	add_state(PATTERN, init_pattern, read_pattern, check_pattern);
	
	add_change_state(BEGIN, "sender", STREAM);
	add_change_state(STREAM, "ip", IP);
	add_change_state(STREAM, "tcp", TCP);
	add_change_state(STREAM, "udp", UDP);
	add_change_state(STREAM, "icmp", ICMP);
	add_change_state(STREAM, "icmp6", ICMP6);
	add_change_state(STREAM, "ip6", IP6);
	add_change_state(STREAM, "traffic", TRAFFIC);
	add_change_state(TRAFFIC, "pattern", PATTERN);
	
}

void add_state(int new_state, void *init_f, void *read_f, void *check_f) {
		
		struct s_node *node;
		node = &head;
		
		/* goto end of link list */
		while(node->next != NULL)
			node = node->next;
			
		node->next = (struct s_node*) cmalloc(sizeof(struct s_node));
		node = node->next;
		node->state = new_state;
		
		node->init = init_f;
		node->read = read_f;
		node->check = check_f;
		node->next = NULL;	
}

void add_change_state(int from_state, char *keyword, int to_state) {
	struct s_change *change;
	change = &change_head;
	
	while(change->next != NULL) 
		change = change->next;
		
	change->next = (struct s_change*) cmalloc(sizeof(struct s_change));
	change = change->next;
	change->from_state = from_state;
	change->keyword = keyword;
	change->to_state = to_state;
	change->next = NULL;
}

int get_next_state(int cur_state, char *keyword) {
	struct s_change *change = &change_head;
	
//	fprintf(stderr, "got state %i and word %s\n", cur_state, keyword);
	
	change = change->next;
	while(change != NULL) {
		if(cur_state == change->from_state)
			if(strcmp(keyword, change->keyword) == 0) {
//				fprintf(stderr, "changing state to %i\n", change->to_state);
				return change->to_state;
			}
				
		change = change->next;
	}
//	fprintf(stderr, "not changing...\n");
	return -1;
}

struct s_node* get_node(int cur_state) {
	struct s_node *node = &head;
	node = node->next;
//	fprintf(stderr, "should get state %i\n", cur_state);
	while(node != NULL) {
		if(cur_state == node->state) {
			return node;
		}
//		fprintf(stderr, "node->state is %i ..next\n", node->state);
		node = node->next;	
	}
//	fprintf(stderr, "state not found %i\n", cur_state);
	return NULL;
}
/*
BOOL exec_state(struct s_automata *automata, char *word) {
		
	struct s_node *node = &head;
	node = node->next;
	while(node != NULL) {
		if(automata.state == node->state) {
			get_pair_value(val, fp);
			node->read(word, val, conf, &automata);
			return TRUE;
		}
		node = node->next;
	}
	return FALSE;
}
*/

void reset_automata(struct s_automata *automata) {
	automata->network_defined = FALSE;
	automata->transport_defined = FALSE;
	automata->traffic_defined = FALSE;
	automata->reset = FALSE;
	automata->state = BEGIN;
}


/* -------------------------------------------------------------------------------------
 				functions for parsing
----------------------------------------------------------------------------------------*/



/*************************************************************************
 *	Skips white characters and some other characters that are
 *	not of interest. Comments ( starting with # ) are skipped as well.
 *************************************************************************/
void skip(FILE *fp) {
	int c;
	BOOL check_white = TRUE;

	while(check_white) {
		fp_line--;
		do {
			c = fgetc(fp);
			fp_line++;
			/* skip these characters */
			while(c == ' ' || c == '\t' || c == '{' ||  c == ';') {
				c = fgetc(fp);
			}
		} while(c == '\n'); /* skip newline */
		/* while-post-condition: character found that we do not want to skip */

		check_white = FALSE;
		
		/* if we found a comment skip it */
		if(c == '#') {
			while(c != '\n' && c != EOF) {
				c = fgetc(fp);
			}
			fp_line++;
			check_white = TRUE; /* we have to check for white characters after
								   the comment again */
		}
	}

	/* give character back to stream */
	ungetc(c, fp);

}


/*************************************************************************
 *	Finds the next word and places it into *word. First all white characters
 *	and other characters that are not of interest are skipped (by calling
 *	the function skip). It then copys the first word it finds into *word.
 *************************************************************************/
int get_next_word(char *word, FILE *fp) {
	char buf[MAXWORDSIZE];
	int c;
	int i = 0;
//	char *str;

	skip(fp);

	c = fgetc(fp);
	if(c == EOF)
		return EOF;

	/* the characters in the while condition mark the end of a word */
	//while(c != ' ' && c != '\t' && c != '{' && c != ';' && c != '=' && c != '\n' && c != EOF) {
	do {
		buf[i] = c;
		i++;
		c = fgetc(fp);
	}while(c != ' ' && c != '\t' && c != '{' && c != ';' && c != '=' && c != '\n' && c != '}' && c != EOF);
	ungetc(c, fp);

	memcpy(word, buf, i);
	word[i] = '\0';
	
	/* do not return EOF since word is not emty */
	return 1;
}


/*******************************************************************************************
 *	Trys to find a word that is a 'pair value' of another word.
 *	Example:	port = 25;
 *			Here the word '25' is a pair value of word 'port'.
 *	The function assumes that the file pointer fp points to the first character that is
 *	located behind the first word of the pair value (behind 'port'). The function
 *	searches for a '=' character and places the word that comes after
 *	into *val.
 ********************************************************************************************/
void get_pair_value(char *val, FILE *fp) {

	char c;
	skip(fp);

	c = fgetc(fp);
	if(c == EOF) {
		ungetc(c, fp);
	}

	else if(c == '=') {
		get_next_word(val, fp);
	}
	else {
		ungetc(c, fp);
	}

}


