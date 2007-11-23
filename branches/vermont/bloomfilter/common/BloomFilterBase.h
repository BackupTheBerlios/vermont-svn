#ifndef _BLOOMFILTER_BASE_H_
#define _BLOOMFILTER_BASE_H_

#ifdef HAVE_GSL

#include <gsl/gsl_rng.h>
#include <stdint.h>
#include <cstring>
#include <ctime>


/* GenericKey class holding uint8_t* input for BloomFilter hash functions */
template<unsigned size> class GenericKey
{
	public:
		unsigned len;
		uint8_t data[size];

		GenericKey() : len(0) {
			memset(data,0,sizeof(data));
		}

		void set(uint8_t *input, unsigned inputlen)
		{
			len = inputlen<sizeof(data)?inputlen:sizeof(data);
			memcpy(&data, input, len);
		}

		void reset()
		{
			len = 0;
			memset(data,0,sizeof(data));
		}

		void append(uint8_t *input, unsigned inputlen)
		{
			if(len<sizeof(data)) {
				if((len+inputlen) < sizeof(data)) {
					memcpy(&(data[len]), input, inputlen);
					len = len + inputlen;
				} else {
					memcpy(&(data[len]), input, sizeof(data)-len);
					len = sizeof(data);
				}
			}
		}

		bool operator<(const GenericKey& other) const 
		{
			if(len < other.len)
				return true;
			else if(len > other.len)
				return false;
			else
				return memcmp(data, other.data, len)<0?true:false;
		}
};


/* QuintupleKey class holding input for BloomFilter hash functions */
class QuintupleKey
{
	public:
		uint8_t data[15];
		const unsigned len;

		struct Quintuple
		{
			uint32_t srcIp;
			uint32_t dstIp;
			uint8_t proto;
			uint16_t srcPort;
			uint16_t dstPort;
		};

		QuintupleKey() : len(15)
		{
			memset(data,0,sizeof(data));
		}

		inline Quintuple* getQuintuple()
		{
			return (Quintuple*)data;
		}

		void reset()
		{
			memset(data,0,sizeof(data));
		}

		bool operator<(const QuintupleKey& other) const 
		{
			return memcmp(data, other.data, len)<0?true:false;
		}
};



/**
 * BloomFilterBase provides hash functions for filters and is the interface for all type of 
 * BloomFilters. The class needs a template parameter which has to provide the following
 * methods/types:
 *
 * class MyT {
 * public:
 * 	MyT(size_t size);
 *	typedef ValueType myDesiredType;
 *	void clear();
 *	myDesiredType get(uint8_t* data, size_t len);
 *	void set(uint8_t* data, size_t len, myDesiredType value);
 * }
 */
template <class T>
class BloomFilterBase
{
	public:
		BloomFilterBase(unsigned hashFunctions, unsigned filterSize) : hf_list(NULL),
			hf_number(hashFunctions), filterSize_(filterSize), filter(filterSize)
		{
			r = gsl_rng_alloc (gsl_rng_fishman18);
			hf_number = hashFunctions;
			initHF();
			clear();
		}

		virtual ~BloomFilterBase()
		{
			free(hf_list);
			gsl_rng_free(r);
		}

		virtual typename T::ValueType get(uint8_t* data, size_t len) const = 0;
		virtual void set(uint8_t* data, size_t len, typename T::ValueType) = 0;

		size_t filterSize() const {
			return filterSize_;
		}

	//protected:
		void initHF()
		{
			free(hf_list);
			hf_list = NULL;
			if(hf_number > 0)
			{
				hf_list = (hash_params*) calloc(sizeof(hash_params), hf_number);
				srand(time(0));
				for(unsigned i=0; i < hf_number; i++) {
					hf_list[i].seed = rand();
				}
			}
		}

		uint32_t hashU(uint8_t* input, uint16_t len, uint32_t max, uint32_t seed) const
		{
			uint32_t random;
			uint32_t result = 0;
			gsl_rng_set(r, seed);
			for(unsigned i = 0; i < len; i++) {
				random = gsl_rng_get (r);
				result = (random*result + input[i]);
			}
			return result % max;
		}

		int32_t ggT(uint32_t m, uint32_t n)
		{
			uint32_t z;
			while (n>0) {
				z=n;
				n=m%n;
				m=z;
			}
			return m;
		}

		void clear()
		{
			filter.clear();
		}


		gsl_rng * r;

		struct hash_params
		{
			uint32_t seed;
		};

		hash_params* hf_list;
		unsigned hf_number;
		size_t filterSize_;
		T filter;
};



#endif

#endif
