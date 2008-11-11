#ifndef ELEMENT_H_
#define ELEMENT_H_


template<typename T>
class Element{
public:

	Element* next; //next Element in list
	Element* prev; //previous Element in list
	T bucket;
	Element(T buck){bucket = buck; prev = 0; next = 0;}
};
#endif
