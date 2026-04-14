#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <limits>
#include <cmath>
#include <cstring>
#include <cstdlib>
#include <filesystem>
#include <system_error>
#include <GL/glew.h>
#ifndef SDL_MAIN_HANDLED
#define SDL_MAIN_HANDLED
#endif
#if __has_include(<SDL2/SDL.h>)
#include <SDL2/SDL.h>
#elif __has_include(<SDL.h>)
#include <SDL.h>
#else
#error "SDL2 headers were not found. Install SDL2 development package."
#endif
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_opengl3.h>

void menu(int item);
GLuint initshaders(GLenum type, const char *filename);
GLuint initprogram(GLuint, GLuint);
void display();
void runMainLoop();
void shutdownInterface();

GLuint  shaderprogram;

static SDL_Window *g_window = NULL;
static SDL_GLContext g_gl_context = NULL;
static bool g_running = true;

// width and height of the window.
int window_w = 600, window_h = 400;

//Variables and their values for the camera setup.
myPoint3D camera_eye(0, 0, 2);
myVector3D camera_up(0, 1, 0);
myVector3D camera_forward(0, 0, -1);

float fovy = 45.0f;
float zNear = 0.1f;
float zFar = 6000;

int frame = 0, timebase = 0;
float fps = 0;

int button_pressed = 0; // 1 if a button is currently being pressed.
int mouse_pos[2] = { 0, 0 };

#define NUM_BUFFERS 16
vector<GLuint> buffers(NUM_BUFFERS, 0);
vector<GLuint> vaos(NUM_BUFFERS, 0);
unsigned int num_triangles;

enum {
	BUFFER_VERTICES = 0, BUFFER_NORMALS_PERFACE, BUFFER_NORMALS_PERVERTEX, BUFFER_VERTICESFORNORMALDRAWING,
	BUFFER_INDICES_TRIANGLES, BUFFER_INDICES_EDGES, BUFFER_INDICES_VERTICES,
	BUFFER_OVERLAY_VERTICES, BUFFER_OVERLAY_NORMALS
};
enum { VAO_TRIANGLES_NORMSPERVERTEX = 0, VAO_TRIANGLES_NORMSPERFACE, VAO_EDGES, VAO_VERTICES, VAO_NORMALS, VAO_OVERLAY };


bool smooth = false; //smooth = true means smooth normals, default false means face-wise normals.
bool drawmesh = true;
bool drawwireframe = false;
bool drawmeshvertices = false;
bool drawsilhouette = false;
bool drawnormals = false;

// ImGui context menu variables
static bool g_show_imgui_menu = false;
static float g_imgui_menu_x = 0.0f;
static float g_imgui_menu_y = 0.0f;

static std::filesystem::path g_executable_path;

static bool file_exists(const std::filesystem::path &path)
{
	std::error_code ec;
	return std::filesystem::is_regular_file(path, ec);
}

static string resolve_resource_path(const string &relative)
{
	std::vector<std::filesystem::path> candidates;
	const std::filesystem::path rel(relative);
	candidates.push_back(rel);

	std::error_code ec;
	const std::filesystem::path cwd = std::filesystem::current_path(ec);
	if (!ec)
	{
		candidates.push_back(cwd / rel);
		candidates.push_back(cwd / "myproj" / rel);
	}

	if (!g_executable_path.empty())
	{
		const std::filesystem::path exe_dir = g_executable_path.parent_path();
		candidates.push_back(exe_dir / rel);
		candidates.push_back(exe_dir / ".." / rel);
		candidates.push_back(exe_dir / ".." / "myproj" / rel);
		candidates.push_back(exe_dir / ".." / ".." / "myproj" / rel);
	}

	for (size_t i = 0; i < candidates.size(); i++)
	{
		if (file_exists(candidates[i]))
			return candidates[i].lexically_normal().string();
	}

	return relative;
}

static bool ray_intersects_triangle(const glm::vec3 &origin, const glm::vec3 &dir,
	const glm::vec3 &v0, const glm::vec3 &v1, const glm::vec3 &v2, float &t_out)
{
	const float EPS = 1e-7f;
	const glm::vec3 edge1 = v1 - v0;
	const glm::vec3 edge2 = v2 - v0;
	const glm::vec3 pvec = glm::cross(dir, edge2);
	const float det = glm::dot(edge1, pvec);

	if (std::fabs(det) < EPS) return false;
	const float inv_det = 1.0f / det;

	const glm::vec3 tvec = origin - v0;
	const float u = glm::dot(tvec, pvec) * inv_det;
	if (u < 0.0f || u > 1.0f) return false;

	const glm::vec3 qvec = glm::cross(tvec, edge1);
	const float v = glm::dot(dir, qvec) * inv_det;
	if (v < 0.0f || u + v > 1.0f) return false;

	const float t = glm::dot(edge2, qvec) * inv_det;
	if (t <= EPS) return false;

	t_out = t;
	return true;
}

static bool build_picking_ray(int x, int y, glm::vec3 &ray_origin, glm::vec3 &ray_direction)
{
	if (window_w <= 0 || window_h <= 0) return false;

	const float ndc_x = (2.0f * static_cast<float>(x) / static_cast<float>(window_w)) - 1.0f;
	const float ndc_y = (2.0f * static_cast<float>(y) / static_cast<float>(window_h)) - 1.0f;

	const glm::mat4 projection_matrix = glm::perspective(glm::radians(fovy), static_cast<float>(window_w) / static_cast<float>(window_h), zNear, zFar);
	const glm::mat4 view_matrix = glm::lookAt(
		glm::vec3(camera_eye.X, camera_eye.Y, camera_eye.Z),
		glm::vec3(camera_eye.X + camera_forward.dX, camera_eye.Y + camera_forward.dY, camera_eye.Z + camera_forward.dZ),
		glm::vec3(camera_up.dX, camera_up.dY, camera_up.dZ));

	const glm::mat4 inv_view_projection = glm::inverse(projection_matrix * view_matrix);
	const glm::vec4 near_clip(ndc_x, ndc_y, -1.0f, 1.0f);
	const glm::vec4 far_clip(ndc_x, ndc_y, 1.0f, 1.0f);

	glm::vec4 near_world = inv_view_projection * near_clip;
	glm::vec4 far_world = inv_view_projection * far_clip;
	if (std::fabs(near_world.w) < 1e-7f || std::fabs(far_world.w) < 1e-7f) return false;

	near_world /= near_world.w;
	far_world /= far_world.w;

	ray_origin = glm::vec3(near_world);
	ray_direction = glm::normalize(glm::vec3(far_world - near_world));
	return true;
}


void makeBuffers(myMesh *input_mesh)
{
	vector <GLfloat> verts; verts.clear();
	vector <GLfloat> norms_per_face; norms_per_face.clear();
	vector <GLfloat> norms; norms.clear();
	vector <GLfloat> verts_and_normals; verts_and_normals.clear();

	num_triangles = 0;
	unsigned int index = 0;
	for (unsigned int i = 0; i < input_mesh->faces.size(); i++)
	{
		myHalfedge *e = input_mesh->faces[i]->adjacent_halfedge->next;
		do {
			verts.push_back((GLfloat)input_mesh->faces[i]->adjacent_halfedge->source->point->X);
			verts.push_back((GLfloat)input_mesh->faces[i]->adjacent_halfedge->source->point->Y);
			verts.push_back((GLfloat)input_mesh->faces[i]->adjacent_halfedge->source->point->Z);
			input_mesh->faces[i]->adjacent_halfedge->source->index = index;
			index++;

			verts.push_back((GLfloat)e->source->point->X);
			verts.push_back((GLfloat)e->source->point->Y);
			verts.push_back((GLfloat)e->source->point->Z);
			e->source->index = index;
			index++;

			verts.push_back((GLfloat)e->next->source->point->X);
			verts.push_back((GLfloat)e->next->source->point->Y);
			verts.push_back((GLfloat)e->next->source->point->Z);
			e->next->source->index = index;
			index++;

			norms_per_face.push_back((GLfloat)input_mesh->faces[i]->normal->dX);
			norms_per_face.push_back((GLfloat)input_mesh->faces[i]->normal->dY);
			norms_per_face.push_back((GLfloat)input_mesh->faces[i]->normal->dZ);

			norms_per_face.push_back((GLfloat)input_mesh->faces[i]->normal->dX);
			norms_per_face.push_back((GLfloat)input_mesh->faces[i]->normal->dY);
			norms_per_face.push_back((GLfloat)input_mesh->faces[i]->normal->dZ);

			norms_per_face.push_back((GLfloat)input_mesh->faces[i]->normal->dX);
			norms_per_face.push_back((GLfloat)input_mesh->faces[i]->normal->dY);
			norms_per_face.push_back((GLfloat)input_mesh->faces[i]->normal->dZ);


			norms.push_back((GLfloat)input_mesh->faces[i]->adjacent_halfedge->source->normal->dX);
			norms.push_back((GLfloat)input_mesh->faces[i]->adjacent_halfedge->source->normal->dY);
			norms.push_back((GLfloat)input_mesh->faces[i]->adjacent_halfedge->source->normal->dZ);

			norms.push_back((GLfloat)e->source->normal->dX);
			norms.push_back((GLfloat)e->source->normal->dY);
			norms.push_back((GLfloat)e->source->normal->dZ);

			norms.push_back((GLfloat)e->next->source->normal->dX);
			norms.push_back((GLfloat)e->next->source->normal->dY);
			norms.push_back((GLfloat)e->next->source->normal->dZ);

			num_triangles++;
			e = e->next;
		} while (e->next != input_mesh->faces[i]->adjacent_halfedge);
	}

	for (unsigned int i = 0; i < input_mesh->vertices.size(); i++)
	{
		verts_and_normals.push_back((GLfloat)input_mesh->vertices[i]->point->X);
		verts_and_normals.push_back((GLfloat)input_mesh->vertices[i]->point->Y);
		verts_and_normals.push_back((GLfloat)input_mesh->vertices[i]->point->Z);

		verts_and_normals.push_back((GLfloat)(input_mesh->vertices[i]->point->X + input_mesh->vertices[i]->normal->dX / 20.0f));
		verts_and_normals.push_back((GLfloat)(input_mesh->vertices[i]->point->Y + input_mesh->vertices[i]->normal->dY / 20.0f));
		verts_and_normals.push_back((GLfloat)(input_mesh->vertices[i]->point->Z + input_mesh->vertices[i]->normal->dZ / 20.0f));
	}

	vector <GLuint> indices_edges;
	for (unsigned int i = 0; i<input_mesh->halfedges.size(); i++)
	{
		if (input_mesh->halfedges[i] == NULL || input_mesh->halfedges[i]->next->next == NULL) continue;
		indices_edges.push_back(input_mesh->halfedges[i]->source->index);
		indices_edges.push_back(input_mesh->halfedges[i]->next->source->index);
	}

	vector <GLuint> indices_vertices;
	for (unsigned int i = 0; i<input_mesh->vertices.size(); i++)
		indices_vertices.push_back(input_mesh->vertices[i]->index);



	glDeleteBuffers(NUM_BUFFERS, &buffers[0]);
	glDeleteVertexArrays(NUM_BUFFERS, &vaos[0]);

	buffers.assign(buffers.size(), 0);
	vaos.assign(vaos.size(), 0);

	glGenBuffers(NUM_BUFFERS, &buffers[0]);

	glBindBuffer(GL_ARRAY_BUFFER, buffers[BUFFER_VERTICES]);
	glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(GLfloat), verts.empty() ? nullptr : verts.data(), GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, buffers[BUFFER_NORMALS_PERVERTEX]);
	glBufferData(GL_ARRAY_BUFFER, norms.size() * sizeof(GLfloat), norms.empty() ? nullptr : norms.data(), GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, buffers[BUFFER_NORMALS_PERFACE]);
	glBufferData(GL_ARRAY_BUFFER, norms_per_face.size() * sizeof(GLfloat), norms_per_face.empty() ? nullptr : norms_per_face.data(), GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, buffers[BUFFER_VERTICESFORNORMALDRAWING]);
	glBufferData(GL_ARRAY_BUFFER, verts_and_normals.size() * sizeof(GLfloat), verts_and_normals.empty() ? nullptr : verts_and_normals.data(), GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffers[BUFFER_INDICES_EDGES]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices_edges.size() * sizeof(GLuint), indices_edges.empty() ? nullptr : indices_edges.data(), GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffers[BUFFER_INDICES_VERTICES]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices_vertices.size() * sizeof(GLuint), indices_vertices.empty() ? nullptr : indices_vertices.data(), GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, buffers[BUFFER_OVERLAY_VERTICES]);
	glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, buffers[BUFFER_OVERLAY_NORMALS]);
	glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);


	glGenVertexArrays(NUM_BUFFERS, &vaos[0]);

	glBindVertexArray(vaos[VAO_TRIANGLES_NORMSPERVERTEX]);
	glBindBuffer(GL_ARRAY_BUFFER, buffers[BUFFER_VERTICES]);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, buffers[BUFFER_NORMALS_PERVERTEX]);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(1);
	glBindVertexArray(0);

	glBindVertexArray(vaos[VAO_TRIANGLES_NORMSPERFACE]);
	glBindBuffer(GL_ARRAY_BUFFER, buffers[BUFFER_VERTICES]);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, buffers[BUFFER_NORMALS_PERFACE]);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(1);
	glBindVertexArray(0);

	glBindVertexArray(vaos[VAO_EDGES]);
	glBindBuffer(GL_ARRAY_BUFFER, buffers[BUFFER_VERTICES]);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, buffers[BUFFER_NORMALS_PERVERTEX]);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffers[BUFFER_INDICES_EDGES]);
	glBindVertexArray(0);

	glBindVertexArray(vaos[VAO_VERTICES]);
	glBindBuffer(GL_ARRAY_BUFFER, buffers[BUFFER_VERTICES]);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, buffers[BUFFER_NORMALS_PERVERTEX]);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffers[BUFFER_INDICES_VERTICES]);
	glBindVertexArray(0);

	glBindVertexArray(vaos[VAO_NORMALS]);
	glBindBuffer(GL_ARRAY_BUFFER, buffers[BUFFER_VERTICESFORNORMALDRAWING]);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);
	glBindVertexArray(0);

	glBindVertexArray(vaos[VAO_OVERLAY]);
	glBindBuffer(GL_ARRAY_BUFFER, buffers[BUFFER_OVERLAY_VERTICES]);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, buffers[BUFFER_OVERLAY_NORMALS]);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(1);
	glBindVertexArray(0);
}


bool PickedPoint(int x, int y)
{
	if ((x < 0) || (window_w <= x) || (y < 0) || (window_h <= y)) return false;
	if (m == NULL || m->faces.empty()) return false;

	glm::vec3 ray_origin;
	glm::vec3 ray_direction;
	if (!build_picking_ray(x, y, ray_origin, ray_direction)) return false;

	float best_t = std::numeric_limits<float>::max();
	myFace *best_face = NULL;
	glm::vec3 best_hit(0.0f);

	for (vector<myFace *>::iterator fit = m->faces.begin(); fit != m->faces.end(); ++fit)
	{
		myFace *face = *fit;
		if (face == NULL || face->adjacent_halfedge == NULL) continue;

		myHalfedge *e0 = face->adjacent_halfedge;
		if (e0->source == NULL || e0->source->point == NULL || e0->next == NULL) continue;

		const glm::vec3 p0(
			static_cast<float>(e0->source->point->X),
			static_cast<float>(e0->source->point->Y),
			static_cast<float>(e0->source->point->Z));

		myHalfedge *e = e0->next;
		size_t guard = 0;
		while (e != NULL && e->next != NULL && e->next != e0 && guard < 100000)
		{
			if (e->source != NULL && e->source->point != NULL &&
				e->next->source != NULL && e->next->source->point != NULL)
			{
				const glm::vec3 p1(
					static_cast<float>(e->source->point->X),
					static_cast<float>(e->source->point->Y),
					static_cast<float>(e->source->point->Z));
				const glm::vec3 p2(
					static_cast<float>(e->next->source->point->X),
					static_cast<float>(e->next->source->point->Y),
					static_cast<float>(e->next->source->point->Z));

				float t = 0.0f;
				if (ray_intersects_triangle(ray_origin, ray_direction, p0, p1, p2, t) && t < best_t)
				{
					best_t = t;
					best_face = face;
					best_hit = ray_origin + ray_direction * t;
				}
			}

			e = e->next;
			guard++;
		}
	}

	if (best_face == NULL) return false;

	closest_face = best_face;
	if (pickedpoint != NULL)
	{
		pickedpoint->X = best_hit.x;
		pickedpoint->Y = best_hit.y;
		pickedpoint->Z = best_hit.z;
	}

	return true;
}


static void print_controls()
{
	cout
		<< "Controls:\n"
		<< "  Mouse drag: orbit camera\n"
		<< "  Right click: open menu\n"
		<< "  Ctrl + Left click: pick point + closest vertex\n"
		<< "  Mouse wheel / Up/Down: zoom\n"
		<< "  Left/Right arrows: rotate view\n"
		<< "  H/F1: print controls, Esc: exit\n";
}

// This function is called when a mouse button is pressed/released.
void mouse(int button, int state, int x, int y, Uint16 modifiers)
{
	mouse_pos[0] = x;
	mouse_pos[1] = window_h - y;

	if (button == SDL_BUTTON_RIGHT && state == SDL_PRESSED)
	{
		button_pressed = 0;
		g_show_imgui_menu = true;
		g_imgui_menu_x = static_cast<float>(x);
		g_imgui_menu_y = static_cast<float>(y);
		return;
	}

	if (g_show_imgui_menu)
	{
		// Let ImGui handle it
		return;
	}

	button_pressed = (state == SDL_PRESSED && button == SDL_BUTTON_LEFT) ? 1 : 0;
	if (state != SDL_PRESSED) return;

	if (button == SDL_BUTTON_LEFT && (modifiers & KMOD_CTRL))
	{
		if (pickedpoint != NULL)
		{
			delete pickedpoint;
			pickedpoint = NULL;
		}

		pickedpoint = new myPoint3D();
		if (!PickedPoint(x, window_h - y))
		{
			delete pickedpoint;
			pickedpoint = NULL;
		}
		else
		{
			menu(MENU_SELECTVERTEX);
		}
	}
}

// This function is called when the mouse is dragged.
void mousedrag(int x, int y, Uint16 modifiers)
{
	if (g_show_imgui_menu) return;

	y = window_h - y;

	const int dx = x - mouse_pos[0];
	const int dy = y - mouse_pos[1];

	mouse_pos[0] = x;
	mouse_pos[1] = y;

	if ((modifiers & KMOD_SHIFT) != 0) return;
	if ((dx == 0 && dy == 0) || !button_pressed) return;

	const double vx = static_cast<double>(dx) / static_cast<double>(window_w);
	const double vy = static_cast<double>(dy) / static_cast<double>(window_h);
	const double theta = 4.0 * (fabs(vx) + fabs(vy));

	myVector3D camera_right = camera_forward.crossproduct(camera_up);
	camera_right.normalize();

	const myVector3D move_right = -camera_right * vx;
	const myVector3D move_up = -camera_up * vy;
	const myVector3D tomovein_direction = move_right + move_up;

	myVector3D rotation_axis = tomovein_direction.crossproduct(camera_forward);
	rotation_axis.normalize();

	camera_forward.rotate(rotation_axis, theta);
	camera_up.rotate(rotation_axis, theta);
	camera_eye.rotate(rotation_axis, theta);

	camera_up.normalize();
	camera_forward.normalize();
}

void mouseWheel(int direction)
{
	if (direction > 0) {
		myVector3D step = camera_forward * 0.02;
		camera_eye += step;
	}
	else if (direction < 0) {
		myVector3D step = -camera_forward * 0.02;
		camera_eye += step;
	}
}

// This function is called when an arrow key is pressed.
void keyboard2(SDL_Keycode key) {
	switch (key) {
	case SDLK_UP:
		{
			myVector3D step = camera_forward * 0.01;
			camera_eye += step;
		}
		break;
	case SDLK_DOWN:
		{
			myVector3D step = -camera_forward * 0.01;
			camera_eye += step;
		}
		break;
	case SDLK_LEFT:
		camera_up.normalize();
		camera_forward.rotate(camera_up, 0.01);
		camera_forward.normalize();
		break;
	case SDLK_RIGHT:
		camera_up.normalize();
		camera_forward.rotate(camera_up, -0.01);
		camera_forward.normalize();
		break;
	}
}

void idle()
{
	frame++;
	const int time = static_cast<int>(SDL_GetTicks());

	if (time - timebase > 1000) {
		fps = frame*1000.0f / (time - timebase);
		timebase = time;
		frame = 0;
	}
}

void reshape(int width, int height) {
	if (width <= 0 || height <= 0) return;
	window_w = width;
	window_h = height;
}

// This function is called when a key is pressed.
void keyboard(SDL_Keycode key) {
	if (g_show_imgui_menu)
	{
		if (key == SDLK_ESCAPE)
		{
			g_show_imgui_menu = false;
			return;
		}
	}

	switch (key) {
	case SDLK_ESCAPE:
		menu(MENU_EXIT);
		break;
	case SDLK_h:
	case SDLK_F1:
		print_controls();
		break;
	default:
		break;
	}
}

void initInterface(int argc, char* argv[])
{
	if (argc > 0 && argv != NULL && argv[0] != NULL)
	{
		std::error_code ec;
		g_executable_path = std::filesystem::absolute(std::filesystem::path(argv[0]), ec);
		if (ec) g_executable_path = std::filesystem::path(argv[0]);
	}

	SDL_SetMainReady();
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_TIMER) != 0)
	{
		cerr << "SDL init failed: " << SDL_GetError() << "\n";
		exit(1);
	}

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
#if defined(__APPLE__)
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#else
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
#endif
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);

	g_window = SDL_CreateWindow(
		"Mesh Viewer",
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		window_w, window_h,
		SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI
	);
	if (g_window == NULL)
	{
		cerr << "Failed to create SDL window: " << SDL_GetError() << "\n";
		SDL_Quit();
		exit(1);
	}

	g_gl_context = SDL_GL_CreateContext(g_window);
	if (g_gl_context == NULL)
	{
		cerr << "Failed to create OpenGL context: " << SDL_GetError() << "\n";
		SDL_DestroyWindow(g_window);
		g_window = NULL;
		SDL_Quit();
		exit(1);
	}

	SDL_GL_MakeCurrent(g_window, g_gl_context);
	SDL_GL_SetSwapInterval(1);

	glewExperimental = GL_TRUE;
	GLenum glew_status = glewInit();
	if (glew_status != GLEW_OK) {
		cerr << "GLEW init failed: " << reinterpret_cast<const char *>(glewGetErrorString(glew_status)) << "\n";
		SDL_GL_DeleteContext(g_gl_context);
		SDL_DestroyWindow(g_window);
		g_gl_context = NULL;
		g_window = NULL;
		SDL_Quit();
		exit(1);
	}
	glGetError(); // GLEW can raise a harmless GL_INVALID_ENUM once on core contexts.

	int drawable_w = window_w;
	int drawable_h = window_h;
	SDL_GL_GetDrawableSize(g_window, &drawable_w, &drawable_h);
	reshape(drawable_w, drawable_h);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glClearColor(1, 1, 1, 0);
	glEnable(GL_MULTISAMPLE);

	GLuint vertexshader = initshaders(GL_VERTEX_SHADER, "shaders/light.vert.glsl");
	GLuint fragmentshader = initshaders(GL_FRAGMENT_SHADER, "shaders/light.frag.glsl");
	shaderprogram = initprogram(vertexshader, fragmentshader);

	// Initialize ImGui
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

	ImGui::StyleColorsDark();
	ImGui_ImplSDL2_InitForOpenGL(g_window, g_gl_context);
	ImGui_ImplOpenGL3_Init("#version 150");

	print_controls();
}

void runMainLoop()
{
	timebase = static_cast<int>(SDL_GetTicks());
	frame = 0;
	g_running = true;

	while (g_running)
	{
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			ImGui_ImplSDL2_ProcessEvent(&event);
			switch (event.type)
			{
			case SDL_QUIT:
				g_running = false;
				break;
			case SDL_WINDOWEVENT:
				if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED || event.window.event == SDL_WINDOWEVENT_RESIZED)
				{
					int drawable_w = 0;
					int drawable_h = 0;
					SDL_GL_GetDrawableSize(g_window, &drawable_w, &drawable_h);
					reshape(drawable_w, drawable_h);
				}
				break;
			case SDL_MOUSEBUTTONDOWN:
			case SDL_MOUSEBUTTONUP:
				mouse(event.button.button, event.button.state, event.button.x, event.button.y, SDL_GetModState());
				break;
			case SDL_MOUSEMOTION:
				if (!g_show_imgui_menu && (event.motion.state & SDL_BUTTON_LMASK) != 0)
				{
					mousedrag(event.motion.x, event.motion.y, SDL_GetModState());
				}
				break;
			case SDL_MOUSEWHEEL:
				mouseWheel(event.wheel.y);
				break;
			case SDL_KEYDOWN:
				if (event.key.repeat != 0) break;
				switch (event.key.keysym.sym)
				{
				case SDLK_UP:
				case SDLK_DOWN:
				case SDLK_LEFT:
				case SDLK_RIGHT:
					keyboard2(event.key.keysym.sym);
					break;
				default:
					keyboard(event.key.keysym.sym);
					break;
				}
				break;
			default:
				break;
			}
		}

		idle();
		display();
	}
}

void shutdownInterface()
{
	// Cleanup ImGui
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();

	if (shaderprogram != 0)
	{
		glDeleteProgram(shaderprogram);
		shaderprogram = 0;
	}

	if (!buffers.empty() && buffers[0] != 0)
	{
		glDeleteBuffers(NUM_BUFFERS, &buffers[0]);
		buffers.assign(buffers.size(), 0);
	}

	if (!vaos.empty() && vaos[0] != 0)
	{
		glDeleteVertexArrays(NUM_BUFFERS, &vaos[0]);
		vaos.assign(vaos.size(), 0);
	}

	if (g_gl_context != NULL)
	{
		SDL_GL_DeleteContext(g_gl_context);
		g_gl_context = NULL;
	}

	if (g_window != NULL)
	{
		SDL_DestroyWindow(g_window);
		g_window = NULL;
	}

	SDL_Quit();
}










// This is a basic program to initiate a shader
// The textFileRead function reads in a filename into a string
// programerrors and shadererrors output compilation errors
// initshaders initiates a vertex or fragment shader
// initprogram initiates a program with vertex and fragment shaders

string textFileRead(const char * filename) {
	string str, ret = "";
	ifstream in;
	string resolved = resolve_resource_path(filename);
	in.open(resolved.c_str());
	if (in.is_open()) {
		getline(in, str);
		while (in) {
			ret += str + "\n";
			getline(in, str);
		}
		//    cout << "Shader below\n" << ret << "\n" ; 
		return ret;
	}
	else {
		cerr << "Unable to Open File " << filename << "\n";
		throw 2;
	}
}

void programerrors(const GLint program) {
	GLint length;
	GLchar * log;
	glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
	log = new GLchar[length + 1];
	glGetProgramInfoLog(program, length, &length, log);
	cout << "Compile Error, Log Below\n" << log << "\n";
	delete[] log;
}
void shadererrors(const GLint shader) {
	GLint length;
	GLchar * log;
	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
	log = new GLchar[length + 1];
	glGetShaderInfoLog(shader, length, &length, log);
	cout << "Compile Error, Log Below\n" << log << "\n";
	delete[] log;
}

GLuint initshaders(GLenum type, const char *filename)
{
	// Using GLSL shaders, OpenGL book, page 679 

	GLuint shader = glCreateShader(type);
	GLint compiled;
	string str = textFileRead(filename);
	GLchar * cstr = new GLchar[str.size() + 1];
	const GLchar * cstr2 = cstr; // Weirdness to get a const char
	std::memcpy(cstr, str.c_str(), str.size() + 1);
	glShaderSource(shader, 1, &cstr2, NULL);
	glCompileShader(shader);
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
	if (!compiled) {
		shadererrors(shader);
		throw 3;
	}
	return shader;
}

GLuint initprogram(GLuint vertexshader, GLuint fragmentshader)
{
	GLuint program = glCreateProgram();
	GLint linked;
	glAttachShader(program, vertexshader);
	glAttachShader(program, fragmentshader);
	glLinkProgram(program);
	glGetProgramiv(program, GL_LINK_STATUS, &linked);
	if (linked) glUseProgram(program);
	else {
		programerrors(program);
		throw 4;
	}
	return program;
}
