#ifndef _COUNT_BLOOMFILTER_H_
#define _COUNT_BLOOMFILTER_H_

#ifdef HAVE_GSL

#include "BloomFilter.h"

class CountArray {
	public:
		CountArray() : array(NULL), array_size(0) {}

		CountArray(uint32_t size) : array(NULL) {
			resize(size);
		}

		~CountArray() {
			free(array);
		}

		void resize(uint32_t size);
		void clear();
		inline uint64_t getAndSet(uint32_t index, uint64_t value);
		inline uint64_t get(uint32_t index) const;

	private:
		uint64_t* array;
		uint32_t array_size;
		friend std::ostream & operator << (std::ostream &, const CountArray &);
};


class CountBloomFilter : public HashFunctions {
	friend std::ostream & operator << (std::ostream &, const CountBloomFilter &);

	public:
		CountBloomFilter() : HashFunctions(), filter_size(0) {}

		CountBloomFilter(uint32_t size, unsigned hashfunctions) : HashFunctions(hashfunctions), filter_size(size) 
		{
			filter.resize(size);
		}

		~CountBloomFilter() {}

		void init(uint32_t size, unsigned hashfunctions);
		void clear();
		uint64_t getValue(uint8_t* input, unsigned len) const;
		uint64_t getAndSetValue(uint8_t* input, unsigned len, uint64_t value);

	private:
		CountArray filter;
		uint32_t filter_size;

};

#endif

#endif
