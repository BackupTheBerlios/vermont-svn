/*
 * PSAMP Reference Implementation
 *
 * IPHeaderFilter.cpp
 *
 * Filter by data from IP packet given by (offset, size, comparison, targetvalue)
 *
 * Author: Michael Drueing <michael@drueing.de>
 *
 */

#include "IPHeaderFilter.h"

inline bool IPHeaderFilter::compareValues(int srcvalue, int dstvalue)
{
        switch(m_comparison){
        case CMP_LT:
                return srcvalue < dstvalue;
        case CMP_LE:
                return srcvalue <= dstvalue;
        case CMP_EQ:
                return srcvalue == dstvalue;
        case CMP_GE:
                return srcvalue >= dstvalue;
        case CMP_GT:
                return srcvalue > dstvalue;
        case CMP_NE:
                return srcvalue != dstvalue;
        case CMP_BIT:
                return ((srcvalue & dstvalue) == dstvalue);
        default:
                return 0;
        }
}

inline int IPHeaderFilter::getData(void *data, int size)
{
        switch(size){
        case 1:
                return *((unsigned char *)data);
        case 2:
                return *((unsigned short *)data);
        case 4:
                return *((int *)data);
        default:
                msg(MSG_ERROR, "IPHeaderFilter: invalid Data Size %d", size);
                return 0;
        }
}

bool IPHeaderFilter::processPacket(const Packet *p)
{
        int srcvalue;
        void *start;

        switch(m_header) {
        case 1:
                start=p->ipHeader;
                break;
        case 2:
                start=p->transportHeader;
                break;
        default:
                start=p->ipHeader;
        }

        srcvalue=getData(((char*)start + m_offset), m_size);
        return compareValues(srcvalue, m_value);
}

