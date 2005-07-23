#ifndef CRC16_H
#define CRC16_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint16_t crc16(uint16_t seed, uint16_t length, char* data);


#ifdef __cplusplus
}
#endif


#endif
