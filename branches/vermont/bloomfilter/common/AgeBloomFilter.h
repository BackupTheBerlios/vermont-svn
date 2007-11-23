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

#ifndef _AGE_BLOOMFILTER_H_
#define _AGE_BLOOMFILTER_H_

#ifdef HAVE_GSL

#include <iostream>
#include <gsl/gsl_rng.h>
#include "BloomFilter.h"

typedef uint64_t agetime_t;

class AgeArray {
    friend std::ostream & operator << (std::ostream &, const AgeArray &);

public:
    AgeArray() : array_size(0), array(NULL) {}

    AgeArray(uint32_t size) : array(NULL)
    {
	resize(size);
    }
	
    ~AgeArray()
    {
	free(array);
    }

    void resize(uint32_t size);
    void clear();
    inline agetime_t getAndSet(uint32_t index, uint64_t time);
    inline agetime_t get(uint32_t index) const;
    
private:
    agetime_t* array;
    uint32_t array_size;
};

std::ostream & operator << (std::ostream &, const AgeArray &);


class AgeBloomFilter : public HashFunctions
{

    friend std::ostream & operator << (std::ostream &, const AgeBloomFilter &);

    public:
    AgeBloomFilter() : HashFunctions(), filter_size(0) {}

    AgeBloomFilter(uint32_t size, unsigned hashfunctions) : HashFunctions(hashfunctions), filter_size(size) 
    {
	filter.resize(size);
    }

    ~AgeBloomFilter() {}

    void init(uint32_t size, unsigned hashfunctions);
    void clear();
    agetime_t getLastTime(uint8_t* input, unsigned len, agetime_t time) const;
    agetime_t getAndSetLastTime(uint8_t* input, unsigned len, agetime_t time);

    private:
    AgeArray filter;
    uint32_t filter_size;
};

std::ostream & operator << (std::ostream &, const AgeBloomFilter &);

#endif // HAVE_GSL

#endif // _AGE_BLOOMFILTER_H_
