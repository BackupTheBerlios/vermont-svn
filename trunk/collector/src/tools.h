/** \file
 * Generic constants, data types and functions.
 */
 
#ifndef TOOLS_H
#define TOOLS_H

#include <stdio.h>
#include <stdint.h>


#define ntoh ntohs

/**
 * Constants used for the SetID of IPFIX Sets.
 */
#define IPFIX_SetId_Template					2
#define IPFIX_SetId_OptionsTemplate				3
#define IPFIX_SetId_DataTemplate				4
#define IPFIX_SetId_Data_Start					256

/** 
 * Constants used for the "type" member of FieldInfo
 * These correspond to the values found in [INFO]
 */ 
#define IPFIX_Type_deltaOctetCount            1
#define IPFIX_Type_deltaPacketCount           2
#define IPFIX_Type_protocolIdentifier         4
#define IPFIX_Type_transportSourcePort        7
#define IPFIX_Type_sourceAddressV4            8
#define IPFIX_Type_transportDestinationPort  11
#define IPFIX_Type_destinationAddressV4      12

/**
 * Constants used for fields of type IPFIX_Type_protocolIdentifier.
 * These correspond to the raw values found in IP packets.
 */
#define IPFIX_protocolIdentifier_ICMP         1
#define IPFIX_protocolIdentifier_TCP          6
#define IPFIX_protocolIdentifier_UDP         17
#define IPFIX_protocolIdentifier_RAW        255


#define true   1
#define false  0

#define uint8 uint8_t
#define uint16 uint16_t
#define uint32 uint32_t

typedef uint8  boolean;
typedef uint8  byte;
typedef uint16 SourceID;
typedef uint16 TemplateID;

#define debug(message, ...) fprintf(stderr, "[DEBUG] %s l.%d: " message "\n", \
	__FILE__, __LINE__, __VA_ARGS__)

#define info(message, ...) fprintf(stderr, "[INFO] %s l.%d: " message "\n", \
	__FILE__, __LINE__, __VA_ARGS__)

#define error(message, ...) fprintf(stderr, "[ERROR] %s l.%d: " message "\n", \
	__FILE__, __LINE__, __VA_ARGS__)

#define fatal(message, ...) fprintf(stderr, "[FATAL] %s l.%d: " message "\n", \
	__FILE__, __LINE__, __VA_ARGS__)

#define debugs(message) fprintf(stderr, "[DEBUG] %s l.%d: " message "\n", \
	__FILE__, __LINE__)

#endif

