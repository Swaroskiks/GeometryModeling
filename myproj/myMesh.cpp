#include "myMesh.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <map>
#include <utility>
#include <cstdlib>
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
	for (unsigned int i = 0; i < vertices.size(); i++)
	{
		if (vertices[i] == NULL) continue;
		if (vertices[i]->normal == NULL) vertices[i]->normal = new myVector3D();
		vertices[i]->normal->clear();
	}

	for (unsigned int i = 0; i < faces.size(); i++)
	{
		if (faces[i] == NULL) continue;
		if (faces[i]->normal == NULL) faces[i]->normal = new myVector3D();
		faces[i]->normal->clear();

		myHalfedge *e0 = faces[i]->adjacent_halfedge;
		if (e0 == NULL || e0->next == NULL || e0->next->next == NULL) continue;
		if (e0->source == NULL || e0->next->source == NULL || e0->next->next->source == NULL) continue;
		if (e0->source->point == NULL || e0->next->source->point == NULL || e0->next->next->source->point == NULL) continue;

		myVector3D fn;
		fn.setNormal(e0->source->point, e0->next->source->point, e0->next->next->source->point);
		faces[i]->normal->dX = fn.dX;
		faces[i]->normal->dY = fn.dY;
		faces[i]->normal->dZ = fn.dZ;

		myHalfedge *e = e0;
		do
		{
			if (e->source != NULL && e->source->normal != NULL)
				*(e->source->normal) += fn;
			e = e->next;
		} while (e != NULL && e != e0);
	}

	for (unsigned int i = 0; i < vertices.size(); i++)
	{
		if (vertices[i] == NULL || vertices[i]->normal == NULL) continue;
		double len = vertices[i]->normal->length();
		if (len > 1e-12)
			vertices[i]->normal->normalize();
		else
		{
			vertices[i]->normal->dX = 0.0;
			vertices[i]->normal->dY = 0.0;
			vertices[i]->normal->dZ = 1.0;
		}
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
	/**** TODO ****/
}

//return false if already triangle, true othewise.
bool myMesh::triangulate(myFace *f)
{
	/**** TODO ****/
	return false;
}

