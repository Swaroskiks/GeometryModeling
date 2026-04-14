#include <math.h>

#ifndef __INCLUDEPOINT3D
#define __INCLUDEPOINT3D 

class myVector3D;

class myPoint3D
{
  //The x,y,z coordinates of this point.
public:
	union
	{
		double components[3];
		struct { double X, Y, Z; };
	};


  myPoint3D();

  myPoint3D(double x, double y, double z);
  double dist(const myPoint3D &p1) const;
  myPoint3D operator+(const myVector3D & v1) const;
  myPoint3D operator+(const myPoint3D & v1) const;
  myPoint3D & operator+=(const myVector3D & v1);
  myPoint3D & operator+=(const myPoint3D & v1);

  myPoint3D & operator/=(double d);
  myPoint3D & operator*=(double d);

  myPoint3D operator/(double d) const;
  myPoint3D operator*(double d) const;

  myVector3D operator-(const myPoint3D & p1) const;
  void rotate(const myVector3D & lp, double theta);
  void print(const char *s) const;
  double dist(myPoint3D *p1, myPoint3D *p2);
  double dist(myPoint3D *p1, myPoint3D *p2, myPoint3D *p3);
  void circumcenter(myPoint3D *p1, myPoint3D *p2, myPoint3D *p3, myPoint3D *p4);
};

#endif


