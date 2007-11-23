#ifndef _MIN_BLOOMFILTER_
#define _MIN_BLOOMFILTER_

#ifdef HAVE_GSL

#include "BloomFilterBase.h"

#include "msg.h"

#include <climits>

// work around a gcc "feature": whenever you need to access a variable from BloomFilterBase<T>,
// you need to specify the full namespace, e.g.  BloomFilterBase::hf_numbers

template <class T>
class MinBloomFilter : public BloomFilterBase<T>
{
	public:
		MinBloomFilter(unsigned hashFunctions, size_t filterSize)
			: BloomFilterBase<T>(hashFunctions, filterSize) {}

		virtual ~MinBloomFilter() {}


		virtual typename T::ValueType  get(uint8_t* input, size_t len) const {
			typename T::ValueType  ret = INT_MAX;
			typename T::ValueType  current;
			for(unsigned i=0; i != BloomFilterBase<T>::hf_number; i++) {   
				current = BloomFilterBase<T>::filter.get(
					BloomFilterBase<T>::hashU(input, len, 
						BloomFilterBase<T>::filterSize(), BloomFilterBase<T>::hf_list[i].seed));
				if (current < ret)
					ret = current;
				}
				return ret;
		}

		virtual void set(uint8_t* input, size_t len, typename T::ValueType v) {
			for(unsigned i=0; i != BloomFilterBase<T>::hf_number; i++) {
				BloomFilterBase<T>::filter.set(BloomFilterBase<T>::hashU(input,	len,
					BloomFilterBase<T>::filterSize(), 
					BloomFilterBase<T>::hf_list[i].seed), v);
			}
		}
};

#endif

#endif
