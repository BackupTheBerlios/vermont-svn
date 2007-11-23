#ifdef HAVE_GSL

#include "CountBloomFilter.h"

void CountArray::resize(uint32_t size)
{
    free(array);
    array_size = size;
    array = (uint64_t*)(malloc(size*sizeof(uint64_t)));
    clear();
}

void CountArray::clear()
{
    memset(array, 0, array_size*sizeof(uint64_t));
}

inline uint64_t CountArray::getAndSet(uint32_t index, uint64_t value)
{
    uint64_t ret = 0;
    if(index < array_size) {
	ret = array[index];
	array[index] += value;
    }
    return ret;
}

inline uint64_t CountArray::get(uint32_t index) const
{
    if(index < array_size)
	return array[index];
    else
	return 0;
}

std::ostream & operator << (std::ostream & os, const CountArray & a) 
{
    for(uint32_t i=0; i<a.array_size; i++)
    {
	os << a.get(i) << " ";
    }
    return os;
}


void CountBloomFilter::init(uint32_t size, unsigned hashfunctions)
{
    hf_number = hashfunctions;
    initHF();
    filter_size = size;
    filter.resize(size);
    clear();
}

void CountBloomFilter::clear()
{
    filter.clear();
}

uint64_t CountBloomFilter::getValue(uint8_t* input, unsigned len) const
{
    uint64_t ret = 0;
    uint64_t current;
    for(unsigned i=0; i < hf_number; i++) 
    {
	current = filter.get(hashU(input, len, filter_size, hf_list[i].seed));
	if (current < ret)
		ret = current;
    }
    return ret;
}

uint64_t CountBloomFilter::getAndSetValue(uint8_t* input, unsigned len, uint64_t value)
{
    uint64_t ret = 0;
    uint64_t current;
    for(unsigned i=0; i < hf_number; i++) 
    {
	current = filter.getAndSet(hashU(input, len, filter_size, hf_list[i].seed), value);
	if (current < ret)
		ret = current;    
    }
    return ret;
}


std::ostream & operator << (std::ostream & os, const CountBloomFilter & b) 
{
    os << b.filter;
    return os;
}


#endif
