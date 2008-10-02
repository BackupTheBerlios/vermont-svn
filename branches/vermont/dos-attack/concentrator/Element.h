#ifndef ELEMENT_H_
#define ELEMENT_H_


template<typename T>
class Element{
public:
	//uint32_t hash;
	Element* next;
	Element* prev;
	T bucket;
	Element(T buck){bucket = buck; prev = 0; next = 0;}
};
#endif
