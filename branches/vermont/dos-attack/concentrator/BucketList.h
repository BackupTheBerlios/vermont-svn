
#ifndef BUCKETLIST_H_
#define BUCKETLIST_H_

#include "Bucket.h"
#include "Element.h"
#include <iostream>
//template<typename T>
class BucketList{
public:
	bool isEmpty;
	Element<Bucket*>* head; /**<first Element in list*/
	Element<Bucket*>* tail; /**< last Element in list*/


/*adds the given Element at the end of the list */
	void push(Element<Bucket*>* node){
	if(isEmpty){
	isEmpty = 0;
	head = node;
	tail = head;
	}else{
	Element<Bucket*>* help = tail;
	help->next = node;
	tail = node;
	tail->prev = help;
	}
	};

/*removes the given Element from the list*/
	void remove(Element<Bucket*>* node){
	if(isEmpty)THROWEXCEPTION("List is empty: can't remove Node");

	else{
	if(!node->prev){
		if(!node->next){
		isEmpty = 1;
		head = 0;
		tail = 0;
		}else{
		node->next->prev = 0;
		head = node->next;
		}
	}//if node not prev
	else if(!node->next){
		node->prev->next = 0;
		tail = node->prev;
	}//if node not next
	else{
		node->prev->next = node->next;
		node->next->prev = node->prev;
	}
	delete node;
	}//else
	};

};
#endif /*BUCKETLIST_H_*/
