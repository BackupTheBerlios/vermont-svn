#include <string.h>
#include "common.h"

int is_in(char letter, char* alphabet) {
	int i;
	for (i = 0; i < strlen(alphabet); i++) {
		if (alphabet[i] == letter) return 1;
		}
	return 0;
	}

void rtrim(char* text) {
	while ((*text != 0) && is_in(text[strlen(text)-1], " \n\t")) text[strlen(text)-1] = 0;
	}

char* ltrim(char* text) {
	while ((*text != 0) && is_in(*text, " \n\t")) ++text;
	return text;
	}

char* get_next_token(char** text, char* delim) {
	char* p = *text;

	if (**text == 0) return NULL;

	for (; **text != 0; ++*text) {
		if (is_in(**text, delim)) {
			**text = 0; ++*text;
			while ((**text != 0) && (is_in(**text, delim))) {
				++*text;
				}
			break;
			}
		}
	return p;	
	}

