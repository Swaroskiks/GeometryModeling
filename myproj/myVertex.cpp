#include "myVertex.h"
#include "myVector3D.h"
#include "myHalfedge.h"
#include "myFace.h"

myVertex::myVertex(void)
{
	point = NULL;
	originof = NULL;
	normal = new myVector3D(1.0,1.0,1.0);
}

myVertex::~myVertex(void)
{
	if (point) delete point;
	if (normal) delete normal;
}

void myVertex::computeNormal()
{
	/**** TODO ****/
}
