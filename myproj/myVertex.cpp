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
	if (originof == NULL) return;

	normal->dX = 0.0;
	normal->dY = 0.0;
	normal->dZ = 0.0;

	myHalfedge *h = originof;
	do {
		if (h->adjacent_face != NULL && h->adjacent_face->normal != NULL)
			*normal += *(h->adjacent_face->normal);

		if (h->prev == NULL || h->prev->twin == NULL) break;
		h = h->prev->twin;
	} while (h != originof);

	normal->normalize();
}
