/**************************************************************************/
/*    Copyright (C) 2007 Gerhard Muenz                                    */
/*                                                                        */
/*    This library is free software; you can redistribute it and/or       */
/*    modify it under the terms of the GNU Lesser General Public          */
/*    License as published by the Free Software Foundation; either        */
/*    version 2.1 of the License, or (at your option) any later version.  */
/*                                                                        */
/*    This library is distributed in the hope that it will be useful,     */
/*    but WITHOUT ANY WARRANTY; without even the implied warranty of      */
/*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU   */
/*    Lesser General Public License for more details.                     */
/*                                                                        */
/*    You should have received a copy of the GNU Lesser General Public    */
/*    License along with this library; if not, write to the Free Software  */
/*    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA    */
/**************************************************************************/

#ifdef HAVE_GSL

#include "BloomFilter.h"

const uint8_t bitmask[8] =
{
    0x01, //00000001
    0x02, //00000010
    0x04, //00000100
    0x08, //00001000
    0x10, //00010000
    0x20, //00100000
    0x40, //01000000
    0x80  //10000000
};
    
void Bitmap::resize(size_t size)
{
    free(bitmap);
    len_bits = size;
    len_octets = (size+7)/8;
    bitmap = (uint8_t*)(malloc(len_octets));
    memset(bitmap, 0, len_octets);
}

void Bitmap::clear()
{
    memset(bitmap, 0, len_octets);
}

void Bitmap::set(size_t index)
{
    if(index < len_bits)
	bitmap[index/8] |= bitmask[index%8];
}

bool Bitmap::get(size_t index) const
{
    if(index < len_bits)
	return (bitmap[index/8] & bitmask[index%8]) != 0;
    else
	return false;
}

std::ostream & operator<< (std::ostream & os, const Bitmap & b) 
{
    for(size_t i=0; i<b.len_bits; i++)
    {
	if(b.get(i))
	    os << '1';
	else
	    os << '0';
    }
    return os;
}

void BloomFilter::set(uint8_t* input, size_t len, bool) 
{
    for(unsigned i=0; i < hf_number; i++) {
	filter.set(hashU(input, len, filterSize(), hf_list[i].seed));
    }
}

bool BloomFilter::get(uint8_t* input, size_t len) const
{
    for(unsigned i=0; i < hf_number; i++) {
	if(filter.get(hashU(input, len, filterSize(), hf_list[i].seed)) == false)
	    return false;
    }
    return true;
}

std::ostream & operator << (std::ostream & os, const BloomFilter & b) 
{
    os << b.filter;
    return os;
}

#endif // HAVE_GSL
