#include "myHalfedge.h"

myHalfedge::myHalfedge(void)
{
	source = nullptr;
	adjacent_face = nullptr;
	next = nullptr;
	prev = nullptr;
	twin = nullptr;
}

void myHalfedge::copy(myHalfedge *ie)
{
/**** TODO ****/
}

myHalfedge::~myHalfedge(void)
{
}
