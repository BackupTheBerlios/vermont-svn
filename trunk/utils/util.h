#ifndef UTIL_H
#define UTIL_H

#define FATAL(fmt, args...) { printf(fmt, ##args); exit(1); }
#define ERROR(fmt, args...) printf(fmt, ##args)

#ifdef DEBUG
#define DPRINTF(fmt, args...) printf(fmt, ##args)
#else
#define DPRINTF(fmt, args...)
#endif

#endif

/* data: pointer to raw IP data */
uint16_t ip_transport_offset(void *data)
{
        uint16_t nlen;
        uint8_t *d=(uint8_t *)data;

        /*
         the header length is in the last 4 bits of the first byte
         in 32bit / 4byte multiples
         */
        nlen = (d[0] & 0xf);

        return(nlen << 2);
}

