#ifndef CONFIG_H
#define CONFIG_H

/**
 * Global configuration parameters
 */
typedef struct {
	uint16_t	port;           /**< Port to listen on for UDP datagrams */
	uint16_t	minBufferTime;  /**< Seconds until an aggregated flow will be exported after the last flow was merged in */
	uint16_t	maxBufferTime;  /**< Seconds until an aggregated flow will be exported after it was created */
	} Config;

Config* readConfigFromFile(char* fname);

void destroyConfig(Config* config);

#endif
