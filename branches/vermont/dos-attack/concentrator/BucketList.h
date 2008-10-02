
#ifndef BUCKETLIST_H_
#define BUCKETLIST_H_

#include "Bucket.h"
#include "Element.h"
#include <iostream>
//template<typename T>
class BucketList{
public:
	bool isEmpty;
/*	template<typename K>
	class Element{
		public:
		uint32_t expireTime;
		Element* next;
		Element* prev;
		K* bucket;
		Element(K* buck, uint32_t exptime){bucket = buck; expireTime = exptime; next = NULL; prev = NULL;}
		
	};*/
	Element<Bucket>* head;
	Element<Bucket>* tail;
	void push(Element<Bucket>* node){
//	cerr << "push Node\n";
	if(isEmpty){
	isEmpty = 0;
	head = node;
	tail = head;
	}else{
	Element<Bucket>* help = tail;
	help->next = node;
	tail = node;
	tail->prev = help;
	}
	};
	void remove(Element<Bucket>* node){
//	cerr << "remove Node\n";
	if(isEmpty)THROWEXCEPTION("List is empty: can't remove Node");
	
	else{
	if(!node->prev){
		if(!node->next){
		isEmpty = 1;
		head = 0;
		tail = 0;
		}else{
//		Element<Bucket>* help = node->next;
		node->next->prev = 0;
		head = node->next;
		//delete help;
		}
	}//if node not prev
	else if(!node->next){
//		Element<Bucket>* help = node->prev;
		node->prev->next = 0;
		tail = node->prev;
		//delete help;
	}//if node not next
	else{
//		Element<Bucket>* help = node->prev;
		node->prev->next = node->next;
//		Element<Bucket>* helper = node->next;
		node->next->prev = node->prev;
//		delete help;
//		delete helper;
	}
	delete node;
	}//else
	};
/*	uint32_t getExpireTime(Element<Bucket>* node){return node->expireTime;};
	BucketList(){head = 0; tail = 0; isEmpty = 1;};*/
	//~BucketList();

};
#endif /*BUCKETLIST_H_*/
