/** \file
 * Generic constants, data types and functions.
 */
 
#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdint.h>
#include "ipfix.h"

#define E_OK   0
#define E_FAIL -1

#define true   1
#define false  0

#define uint8 uint8_t
#define uint16 uint16_t
#define uint32 uint32_t

typedef uint8  boolean;
typedef uint8  byte;

#define ntoh ntohs

#define debugf(message, ...) fprintf(stderr, "[DEBUG] %s l.%d: " message "\n", \
	__FILE__, __LINE__, __VA_ARGS__)

#define infof(message, ...) fprintf(stderr, "[INFO] %s l.%d: " message "\n", \
	__FILE__, __LINE__, __VA_ARGS__)

#define errorf(message, ...) fprintf(stderr, "[ERROR] %s l.%d: " message "\n", \
	__FILE__, __LINE__, __VA_ARGS__)

#define fatalf(message, ...) fprintf(stderr, "[FATAL] %s l.%d: " message "\n", \
	__FILE__, __LINE__, __VA_ARGS__)

#define debug(message) fprintf(stderr, "[DEBUG] %s l.%d: " message "\n", \
	__FILE__, __LINE__)


int is_in(char letter, char* alphabet);
void rtrim(char* text);
char* ltrim(char* text);
char* get_next_token(char** text, char* delim);

#endif
