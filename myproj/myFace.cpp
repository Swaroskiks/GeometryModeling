#include "myFace.h"
#include "myVector3D.h"
#include "myHalfedge.h"
#include "myVertex.h"
#include <GL/glew.h>

myFace::myFace(void)
{
	adjacent_halfedge = NULL;
	normal = new myVector3D(1.0, 1.0, 1.0);
}

myFace::~myFace(void)
{
	if (normal) delete normal;
}

void myFace::computeNormal()
{
	if (adjacent_halfedge == NULL) return;

	myHalfedge *e = adjacent_halfedge;
	myPoint3D *p1 = e->source->point;
	myPoint3D *p2 = e->next->source->point;
	myPoint3D *p3 = e->next->next->source->point;

	normal->setNormal(p1, p2, p3);
}
