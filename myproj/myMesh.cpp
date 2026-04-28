#include "myMesh.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <map>
#include <utility>
#include <cstdlib>
#include <cmath>
#include <GL/glew.h>
#include "myVector3D.h"

using namespace std;

myMesh::myMesh(void)
{
}


myMesh::~myMesh(void)
{
	clear();
}

void myMesh::clear()
{
	for (unsigned int i = 0; i < vertices.size(); i++) if (vertices[i]) delete vertices[i];
	for (unsigned int i = 0; i < halfedges.size(); i++) if (halfedges[i]) delete halfedges[i];
	for (unsigned int i = 0; i < faces.size(); i++) if (faces[i]) delete faces[i];

	vector<myVertex *> empty_vertices;    vertices.swap(empty_vertices);
	vector<myHalfedge *> empty_halfedges; halfedges.swap(empty_halfedges);
	vector<myFace *> empty_faces;         faces.swap(empty_faces);
}

void myMesh::checkMesh()
{
	vector<myHalfedge *>::iterator it;
	for (it = halfedges.begin(); it != halfedges.end(); it++)
	{
		if ((*it)->twin == NULL)
			break;
	}
	if (it != halfedges.end())
		cout << "Error! Not all edges have their twins!\n";
	else cout << "Each edge has a twin!\n";
}


bool myMesh::readFile(std::string filename)
{
	clear();

	ifstream fin(filename);
	if (!fin.is_open()) {
		cout << "Unable to open file!\n";
		return false;
	}
	name = filename;

	map<pair<int, int>, myHalfedge *> twin_map;
	string s, t, u;

	while (getline(fin, s))
	{
		if (s.empty()) continue;
		stringstream myline(s);
		myline >> t;
		if (t.empty() || t[0] == '#') {}
		else if (t == "g") {}
		else if (t == "v")
		{
			double x, y, z;
			if (!(myline >> x >> y >> z)) continue;

			myVertex *v = new myVertex();
			v->point = new myPoint3D(x, y, z);
			v->index = static_cast<int>(vertices.size());
			vertices.push_back(v);
		}
		else if (t == "mtllib") {}
		else if (t == "usemtl") {}
		else if (t == "s") {}
		else if (t == "f")
		{
			vector<int> faceids;
			while (myline >> u)
			{
				size_t sep = u.find('/');
				string vid = (sep == string::npos) ? u : u.substr(0, sep);
				if (vid.empty()) continue;

				int idx = atoi(vid.c_str());
				if (idx > 0) idx -= 1;
				else if (idx < 0) idx = static_cast<int>(vertices.size()) + idx;
				else continue;

				if (idx < 0 || idx >= static_cast<int>(vertices.size())) continue;
				faceids.push_back(idx);
			}

			if (faceids.size() < 3) continue;

			myFace *f = new myFace();
			faces.push_back(f);

			vector<myHalfedge *> face_halfedges(faceids.size(), NULL);

			for (size_t i = 0; i < faceids.size(); i++)
			{
				myHalfedge *h = new myHalfedge();
				h->source = vertices[faceids[i]];
				h->adjacent_face = f;
				h->index = static_cast<int>(halfedges.size());
				halfedges.push_back(h);
				face_halfedges[i] = h;

				if (h->source != NULL && h->source->originof == NULL)
					h->source->originof = h;
			}

			for (size_t i = 0; i < face_halfedges.size(); i++)
			{
				face_halfedges[i]->next = face_halfedges[(i + 1) % face_halfedges.size()];
				face_halfedges[i]->prev = face_halfedges[(i + face_halfedges.size() - 1) % face_halfedges.size()];
			}
			f->adjacent_halfedge = face_halfedges[0];

			for (size_t i = 0; i < faceids.size(); i++)
			{
				const int from = faceids[i];
				const int to = faceids[(i + 1) % faceids.size()];

				pair<int, int> reverse_edge(to, from);
				map<pair<int, int>, myHalfedge *>::iterator twin_it = twin_map.find(reverse_edge);
				if (twin_it != twin_map.end())
				{
					face_halfedges[i]->twin = twin_it->second;
					twin_it->second->twin = face_halfedges[i];
					twin_map.erase(twin_it);
				}
				else
				{
					twin_map[pair<int, int>(from, to)] = face_halfedges[i];
				}
			}
		}
	}

	if (vertices.empty() || faces.empty())
	{
		cout << "OBJ contained no usable geometry.\n";
		return false;
	}

	checkMesh();
	normalize();

	return true;
}


void myMesh::computeNormals()
{
	for (unsigned int i = 0; i < faces.size(); i++)
	{
		faces[i]->computeNormal();
	}

	for (unsigned int i = 0; i < vertices.size(); i++)
	{
		vertices[i]->computeNormal();
	}
}

void myMesh::normalize()
{
	if (vertices.size() < 1) return;

	int tmpxmin = 0, tmpymin = 0, tmpzmin = 0, tmpxmax = 0, tmpymax = 0, tmpzmax = 0;

	for (unsigned int i = 0; i < vertices.size(); i++) {
		if (vertices[i]->point->X < vertices[tmpxmin]->point->X) tmpxmin = i;
		if (vertices[i]->point->X > vertices[tmpxmax]->point->X) tmpxmax = i;

		if (vertices[i]->point->Y < vertices[tmpymin]->point->Y) tmpymin = i;
		if (vertices[i]->point->Y > vertices[tmpymax]->point->Y) tmpymax = i;

		if (vertices[i]->point->Z < vertices[tmpzmin]->point->Z) tmpzmin = i;
		if (vertices[i]->point->Z > vertices[tmpzmax]->point->Z) tmpzmax = i;
	}

	double xmin = vertices[tmpxmin]->point->X, xmax = vertices[tmpxmax]->point->X,
		ymin = vertices[tmpymin]->point->Y, ymax = vertices[tmpymax]->point->Y,
		zmin = vertices[tmpzmin]->point->Z, zmax = vertices[tmpzmax]->point->Z;

	double scale = (xmax - xmin) > (ymax - ymin) ? (xmax - xmin) : (ymax - ymin);
	scale = scale > (zmax - zmin) ? scale : (zmax - zmin);

	for (unsigned int i = 0; i < vertices.size(); i++) {
		vertices[i]->point->X -= (xmax + xmin) / 2;
		vertices[i]->point->Y -= (ymax + ymin) / 2;
		vertices[i]->point->Z -= (zmax + zmin) / 2;

		vertices[i]->point->X /= scale;
		vertices[i]->point->Y /= scale;
		vertices[i]->point->Z /= scale;
	}
}


void myMesh::splitFaceTRIS(myFace *f, myPoint3D *p)
{
	/**** TODO ****/
}

void myMesh::splitEdge(myHalfedge *e1, myPoint3D *p)
{

	/**** TODO ****/
}

void myMesh::splitFaceQUADS(myFace *f, myPoint3D *p)
{
	/**** TODO ****/
}


void myMesh::subdivisionCatmullClark()
{
	/**** TODO ****/
}


void myMesh::triangulate()
{
	vector<myFace *> original_faces(faces);
	for (unsigned int i = 0; i < original_faces.size(); i++)
	{
		triangulate(original_faces[i]);
	}
}

bool myMesh::triangulate(myFace *f)
{
	if (f == NULL || f->adjacent_halfedge == NULL) return false;

	vector<myHalfedge*> ring;
	{
		myHalfedge *e = f->adjacent_halfedge;
		size_t guard = 0;
		do { ring.push_back(e); e = e->next; guard++; }
		while (e != f->adjacent_halfedge && guard < 100000);
	}

	int n = (int)ring.size();
	if (n <= 3) return false;

	// On utilise la méthode de Newell pour calculer la normale de la face
	double nx = 0, ny = 0, nz = 0;
	for (int i = 0; i < n; i++) {
		myPoint3D *p0 = ring[i]->source->point;
		myPoint3D *p1 = ring[(i + 1) % n]->source->point;
		nx += (p0->Y - p1->Y) * (p0->Z + p1->Z);
		ny += (p0->Z - p1->Z) * (p0->X + p1->X);
		nz += (p0->X - p1->X) * (p0->Y + p1->Y);
	}
	int skip;
	if (fabs(nx) >= fabs(ny) && fabs(nx) >= fabs(nz)) skip = 0;
	else if (fabs(ny) >= fabs(nx) && fabs(ny) >= fabs(nz)) skip = 1;
	else skip = 2;

	auto get2D = [&](myPoint3D *p) -> pair<double, double> {
		if (skip == 0) return { p->Y, p->Z };
		if (skip == 1) return { p->X, p->Z };
		return { p->X, p->Y };
	};

	double area2 = 0;
	for (int i = 0; i < n; i++) {
		auto p0 = get2D(ring[i]->source->point);
		auto p1 = get2D(ring[(i + 1) % n]->source->point);
		area2 += p0.first * p1.second - p1.first * p0.second;
	}
	double wind = (area2 >= 0) ? 1.0 : -1.0;

	auto cross2D = [](double ax, double ay, double bx, double by, double px, double py) {
		return (bx - ax) * (py - ay) - (by - ay) * (px - ax);
	};

	auto insideTriangle = [&](pair<double,double> p,
	                          pair<double,double> a,
	                          pair<double,double> b,
	                          pair<double,double> c) {
		double d1 = cross2D(a.first,a.second, b.first,b.second, p.first,p.second) * wind;
		double d2 = cross2D(b.first,b.second, c.first,c.second, p.first,p.second) * wind;
		double d3 = cross2D(c.first,c.second, a.first,a.second, p.first,p.second) * wind;
		return d1 > 1e-10 && d2 > 1e-10 && d3 > 1e-10;
	};

	while ((int)ring.size() > 3) {
		int sz = (int)ring.size();
		int ear = -1;

		for (int i = 0; i < sz; i++) {
			int pi = (i - 1 + sz) % sz;
			int ni = (i + 1) % sz;

			auto vp = get2D(ring[pi]->source->point);
			auto vi = get2D(ring[i]->source->point);
			auto vn = get2D(ring[ni]->source->point);

			if (cross2D(vp.first,vp.second, vi.first,vi.second, vn.first,vn.second) * wind <= 1e-10)
				continue;

			bool valid = true;
			for (int j = 0; j < sz && valid; j++) {
				if (j == pi || j == i || j == ni) continue;
				if (insideTriangle(get2D(ring[j]->source->point), vp, vi, vn))
					valid = false;
			}
			if (valid) { ear = i; break; }
		}

		if (ear < 0) ear = 0;

		int prev_i = (ear - 1 + sz) % sz;
		int next_i = (ear + 1) % sz;

		myFace *ear_face = new myFace();
		faces.push_back(ear_face);

	    myHalfedge *diag_back    = new myHalfedge();
		myHalfedge *diag_forward = new myHalfedge();
		halfedges.push_back(diag_back);
		halfedges.push_back(diag_forward);

		diag_back->twin    = diag_forward;
		diag_forward->twin = diag_back;
		diag_back->source    = ring[next_i]->source;
		diag_forward->source = ring[prev_i]->source;

		ring[prev_i]->adjacent_face = ear_face;
		ring[ear]->adjacent_face    = ear_face;
		diag_back->adjacent_face    = ear_face;

		ring[prev_i]->next = ring[ear];    ring[ear]->prev     = ring[prev_i];
		ring[ear]->next    = diag_back;    diag_back->prev     = ring[ear];
		diag_back->next    = ring[prev_i]; ring[prev_i]->prev  = diag_back;
		ear_face->adjacent_halfedge = ring[prev_i];

		diag_forward->adjacent_face = f;
		myHalfedge *before = ring[(prev_i - 1 + sz) % sz];
		before->next         = diag_forward; diag_forward->prev = before;
		diag_forward->next   = ring[next_i]; ring[next_i]->prev = diag_forward;

		vector<myHalfedge*> new_ring;
		new_ring.reserve(sz - 1);
		for (int j = 0; j < sz; j++) {
			if (j == ear) continue;
			new_ring.push_back(j == prev_i ? diag_forward : ring[j]);
		}
		ring = std::move(new_ring);
	}

	f->adjacent_halfedge = ring[0];
	for (int j = 0; j < 3; j++) {
		ring[j]->adjacent_face = f;
		ring[j]->next          = ring[(j + 1) % 3];
		ring[(j + 1) % 3]->prev = ring[j];
	}

	return true;
}

