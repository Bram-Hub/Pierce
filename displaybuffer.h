#ifndef desplaybuffer_h_
#define desplaybuffer_h_

#include <list>
#include "graphTree.h"

class displaybuffer
{
public:
	displaybuffer();
	displaybuffer(unsigned int size);
	~displaybuffer();
	void init(unsigned int size);
	void reinit(unsigned int size);
	graphTree* write(std::list<graphTree*>::iterator &cur, graphTree* newTree);
	std::list<graphTree*>::iterator begin()	{ return buffer.begin(); };
	std::list<graphTree*>::iterator end() { return buffer.end(); };
	void resize(unsigned int size, std::list<graphTree*> &discard);
private:
	std::list<graphTree*> buffer;
	unsigned int bSize;
};

displaybuffer::displaybuffer()
{
	bSize = 1;
}

displaybuffer::displaybuffer(unsigned int size)
{
	bSize = size+1;
}

displaybuffer::~displaybuffer()
{
	//Destroy list and free memory
	std::list<graphTree*>::iterator itr = buffer.begin();
	while (itr != buffer.end())
	{
		delete *itr;
		itr++;
	}
	buffer.clear();
}

void displaybuffer::init(unsigned int size)
{
	//buffer.clear();
	bSize = size+1;
	buffer.push_back(new graphTree());
}

void displaybuffer::reinit(unsigned int size)
{
	//Clean out existing buffer
	std::list<graphTree*>::iterator itr = buffer.begin();
	while (itr != buffer.end())
	{
		delete *itr;
		itr++;
	}
	buffer.clear();

	bSize = size+1;
	buffer.push_back(new graphTree());
}

//This function handles adding a new node to the list.
//This is done after the current node being displayed,
//so anything after that is removed and destroyed.
//This function will also bump the oldest (first) entry
//in the buffer out if the new size exceeds bSize+1.
//It will NOT destroy that node, however -- it returns
//it instead.
//Return value is NULL if the oldest was not bumped.
graphTree* displaybuffer::write(std::list<graphTree*>::iterator &cur, graphTree* newTree)
{
	cur++;
	if (cur != buffer.end())
	{
		std::list<graphTree*>::iterator itr = cur;
		while (itr != buffer.end())
		{
			delete *itr;	//Free memory
			itr++;
		}
		buffer.erase(cur, buffer.end());	//Clear everything after current node
	}
	buffer.push_back(newTree);
	//Make sure the referenced iterator is pointing at the new node
	cur = buffer.begin();
	while ((*cur) != newTree)
	{
		cur++;
	}

	if (buffer.size() > bSize)
	{
		graphTree* front = buffer.front();	//Remove and return the first node
		buffer.pop_front();
		return front;
	}
	else
		return (graphTree*)0;
}

//This function will change the size of the buffer.
//If the new size is larger, nothing special happens,
//but if the new size is smaller, it will probably have
//to remove elements from the beginning of the list.
//If so, it will append those to the end of the list
//given as the argument &discard.
void displaybuffer::resize(unsigned int size, std::list<graphTree*> &discard)
{
	bSize = size+1;

	int sDif = buffer.size() - bSize;

	if (sDif > 0)
	{
		std::list<graphTree*>::iterator itr = buffer.begin();
		while (sDif > 0)
		{
			if (&discard != NULL)
				discard.push_back(*itr);	//Save the elements to be removed
			itr++;
			--sDif;
		}
		buffer.erase(buffer.begin(), itr);
	}
}

#endif