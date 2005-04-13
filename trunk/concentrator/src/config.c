/** \file
 * Global configuration management.
 *
 * Parses a config file into a Config structure which can then be used throughout an application
 *
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "common.h"
#include "config.h"

/**
 * Reads in global configuration options
 * @return NULL if an error occured
 */
Config* readConfigFromFile(char* fname) {
	FILE* f;
	char buf[256];
        char* key;
        char* value;
	Config* config;
	config = (Config*)malloc(sizeof(Config));
	config->port = 1500;
	config->minBufferTime = 5;
	config->maxBufferTime = 15;

	f = fopen(fname, "r");
	if (!f) {
		fatalf("Opening %s failed", fname);
		return NULL;
		}
	while (!feof(f)) {
		fgets(buf, sizeof(buf), f);
		if ((buf[0] == 0) || (buf[0] == ';') || (buf[0] == '#')) continue;
		if (strlen(buf) < 5) continue;

		//debugf("Line '%s'", buf);

		key = strtok(buf, " \t\n\r");
		value = strtok(0, " \t\n\r");

		if (!value) continue;
		
		//debugf("Key '%s', Value '%s'", key, value);

		if (strcasecmp(key, "Port") == 0) config->port = atoi(value);
		if (strcasecmp(key, "MinBufferTime") == 0) config->minBufferTime = atoi(value);
		if (strcasecmp(key, "MaxBufferTime") == 0) config->maxBufferTime = atoi(value);
		}

	fclose(f);

	return config;
	}

void destroyConfig(Config* config) {
	free(config);
	}
