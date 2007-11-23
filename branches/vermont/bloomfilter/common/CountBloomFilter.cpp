#ifdef HAVE_GSL

#include "CountBloomFilter.h"

void CountArray::resize(size_t size)
{
    free(array);
    array_size = size;
    array = (ValueType*)(malloc(size*sizeof(ValueType)));
    clear();
}

void CountArray::clear()
{
    memset(array, 0, array_size*sizeof(ValueType));
}

void CountArray::set(size_t index, ValueType value)
{
    if(index < array_size) {
	array[index] += value;
    }
}

CountArray::ValueType CountArray::get(size_t index) const
{
    if(index < array_size)
	return array[index];
    else
	return 0;
}

std::ostream & operator << (std::ostream & os, const CountArray & a) 
{
    for(size_t i=0; i<a.array_size; i++)
    {
	os << a.get(i) << " ";
    }
    return os;
}

#endif
