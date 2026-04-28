#include <fstream>
#include <GL/glew.h>
#include <sstream>
#include <cstdlib>
#include <limits>
#include <array>
#include <cctype>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#if defined(MESHVIEWER_ENABLE_NFD)
#include <nfd.hpp>
#endif

using namespace std;

#include "myPoint3D.h"
#include "myVector3D.h"
#include "myMesh.h"

enum MENU { MENU_CATMULLCLARK, MENU_DRAWWIREFRAME, MENU_EXIT, MENU_DRAWMESH, MENU_LOOP, MENU_DRAWMESHVERTICES,
	MENU_CONTRACTEDGE, MENU_CONTRACTFACE, MENU_DRAWCREASE, MENU_DRAWSILHOUETTE, 
	MENU_GENERATE, MENU_CUT, MENU_INFLATE, MENU_SELECTEDGE, MENU_SELECTFACE, MENU_SELECTVERTEX,
	MENU_SHADINGTYPE, MENU_SMOOTHEN, MENU_SPLITEDGE, MENU_SPLITFACE, MENU_SELECTCLEAR, 
	MENU_TRIANGULATE, MENU_UNDO, MENU_WRITE, MENU_SIMPLIFY, MENU_DRAWNORMALS, MENU_OPENFILE
};
 
myMesh *m;
myPoint3D *pickedpoint;
myHalfedge *closest_edge;
myVertex *closest_vertex;
myFace *closest_face;

#include "helperFunctions.h"

void clear()
{
	if (pickedpoint != NULL)
	{
		delete pickedpoint;
	}
	closest_edge = NULL;
	closest_vertex = NULL;
	closest_face = NULL;
	pickedpoint = NULL;
}

static void append_overlay_vertex(vector<GLfloat> &dst, myPoint3D *p)
{
	if (p == NULL) return;
	dst.push_back(static_cast<GLfloat>(p->X));
	dst.push_back(static_cast<GLfloat>(p->Y));
	dst.push_back(static_cast<GLfloat>(p->Z));
}

static vector<GLfloat> build_face_overlay_triangles(myFace *face)
{
	vector<GLfloat> tris;
	if (face == NULL || face->adjacent_halfedge == NULL) return tris;

	myHalfedge *e0 = face->adjacent_halfedge;
	if (e0->source == NULL || e0->source->point == NULL || e0->next == NULL) return tris;

	myPoint3D *p0 = e0->source->point;
	myHalfedge *e = e0->next;
	size_t guard = 0;

	while (e != NULL && e->next != NULL && e->next != e0 && guard < 100000)
	{
		if (e->source != NULL && e->source->point != NULL &&
			e->next->source != NULL && e->next->source->point != NULL)
		{
			append_overlay_vertex(tris, p0);
			append_overlay_vertex(tris, e->source->point);
			append_overlay_vertex(tris, e->next->source->point);
		}
		e = e->next;
		guard++;
	}

	return tris;
}

static void draw_overlay_geometry(GLenum mode, const vector<GLfloat> &vertices,
	const array<GLfloat, 4> &color, float line_width = 1.0f, float point_size = 1.0f)
{
	if (vertices.empty() || vaos[VAO_OVERLAY] == 0) return;

	vector<GLfloat> normals(vertices.size(), 0.0f);
	for (size_t i = 2; i < normals.size(); i += 3) normals[i] = 1.0f;

	glUniform1i(glGetUniformLocation(shaderprogram, "type"), 1);
	glUniform4fv(glGetUniformLocation(shaderprogram, "kd"), 1, color.data());

	if (mode == GL_LINES) glLineWidth(line_width);
	if (mode == GL_POINTS) glPointSize(point_size);

	glBindBuffer(GL_ARRAY_BUFFER, buffers[BUFFER_OVERLAY_VERTICES]);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat), vertices.data(), GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, buffers[BUFFER_OVERLAY_NORMALS]);
	glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(GLfloat), normals.data(), GL_DYNAMIC_DRAW);

	glBindVertexArray(vaos[VAO_OVERLAY]);
	glDrawArrays(mode, 0, static_cast<GLsizei>(vertices.size() / 3));
	glBindVertexArray(0);

	glUniform1i(glGetUniformLocation(shaderprogram, "type"), 0);
}

static float hud_ndc_x(float px)
{
	const float w = (window_w > 0) ? static_cast<float>(window_w) : 1.0f;
	return (px / w) * 2.0f - 1.0f;
}

static float hud_ndc_y(float py)
{
	const float h = (window_h > 0) ? static_cast<float>(window_h) : 1.0f;
	return 1.0f - (py / h) * 2.0f;
}

static void hud_add_line(vector<GLfloat> &out, float x1, float y1, float x2, float y2)
{
	out.push_back(hud_ndc_x(x1)); out.push_back(hud_ndc_y(y1)); out.push_back(0.0f);
	out.push_back(hud_ndc_x(x2)); out.push_back(hud_ndc_y(y2)); out.push_back(0.0f);
}

static void hud_add_triangle(vector<GLfloat> &out, float x1, float y1, float x2, float y2, float x3, float y3)
{
	out.push_back(hud_ndc_x(x1)); out.push_back(hud_ndc_y(y1)); out.push_back(0.0f);
	out.push_back(hud_ndc_x(x2)); out.push_back(hud_ndc_y(y2)); out.push_back(0.0f);
	out.push_back(hud_ndc_x(x3)); out.push_back(hud_ndc_y(y3)); out.push_back(0.0f);
}

static void hud_add_segment(vector<GLfloat> &out, int seg, float x, float y, float w, float h)
{
	float x1 = x, y1 = y, x2 = x, y2 = y;
	switch (seg)
	{
	case 0: x1 = x + 0.10f*w; y1 = y + 0.05f*h; x2 = x + 0.90f*w; y2 = y + 0.05f*h; break; // a
	case 1: x1 = x + 0.90f*w; y1 = y + 0.05f*h; x2 = x + 0.90f*w; y2 = y + 0.50f*h; break; // b
	case 2: x1 = x + 0.90f*w; y1 = y + 0.50f*h; x2 = x + 0.90f*w; y2 = y + 0.95f*h; break; // c
	case 3: x1 = x + 0.10f*w; y1 = y + 0.95f*h; x2 = x + 0.90f*w; y2 = y + 0.95f*h; break; // d
	case 4: x1 = x + 0.10f*w; y1 = y + 0.50f*h; x2 = x + 0.10f*w; y2 = y + 0.95f*h; break; // e
	case 5: x1 = x + 0.10f*w; y1 = y + 0.05f*h; x2 = x + 0.10f*w; y2 = y + 0.50f*h; break; // f
	case 6: x1 = x + 0.10f*w; y1 = y + 0.50f*h; x2 = x + 0.90f*w; y2 = y + 0.50f*h; break; // g
	default: return;
	}
	hud_add_line(out, x1, y1, x2, y2);
}

static void hud_add_char(vector<GLfloat> &out, char ch, float x, float y, float w, float h)
{
	const int digit_masks[10] = {
		(1 << 0) | (1 << 1) | (1 << 2) | (1 << 3) | (1 << 4) | (1 << 5),             // 0
		(1 << 1) | (1 << 2),                                                           // 1
		(1 << 0) | (1 << 1) | (1 << 6) | (1 << 4) | (1 << 3),                         // 2
		(1 << 0) | (1 << 1) | (1 << 6) | (1 << 2) | (1 << 3),                         // 3
		(1 << 5) | (1 << 6) | (1 << 1) | (1 << 2),                                    // 4
		(1 << 0) | (1 << 5) | (1 << 6) | (1 << 2) | (1 << 3),                         // 5
		(1 << 0) | (1 << 5) | (1 << 6) | (1 << 2) | (1 << 3) | (1 << 4),             // 6
		(1 << 0) | (1 << 1) | (1 << 2),                                                // 7
		(1 << 0) | (1 << 1) | (1 << 2) | (1 << 3) | (1 << 4) | (1 << 5) | (1 << 6), // 8
		(1 << 0) | (1 << 1) | (1 << 2) | (1 << 3) | (1 << 5) | (1 << 6)              // 9
	};

	if (ch >= '0' && ch <= '9')
	{
		const int mask = digit_masks[ch - '0'];
		for (int s = 0; s < 7; s++) if (mask & (1 << s)) hud_add_segment(out, s, x, y, w, h);
		return;
	}

	switch (std::toupper(static_cast<unsigned char>(ch)))
	{
	case 'V':
		hud_add_line(out, x + 0.12f*w, y + 0.05f*h, x + 0.50f*w, y + 0.95f*h);
		hud_add_line(out, x + 0.88f*w, y + 0.05f*h, x + 0.50f*w, y + 0.95f*h);
		break;
	case 'H':
		hud_add_line(out, x + 0.12f*w, y + 0.05f*h, x + 0.12f*w, y + 0.95f*h);
		hud_add_line(out, x + 0.88f*w, y + 0.05f*h, x + 0.88f*w, y + 0.95f*h);
		hud_add_line(out, x + 0.12f*w, y + 0.50f*h, x + 0.88f*w, y + 0.50f*h);
		break;
	case 'F':
		hud_add_line(out, x + 0.12f*w, y + 0.05f*h, x + 0.12f*w, y + 0.95f*h);
		hud_add_line(out, x + 0.12f*w, y + 0.05f*h, x + 0.90f*w, y + 0.05f*h);
		hud_add_line(out, x + 0.12f*w, y + 0.50f*h, x + 0.78f*w, y + 0.50f*h);
		break;
	case 'P':
		hud_add_line(out, x + 0.12f*w, y + 0.05f*h, x + 0.12f*w, y + 0.95f*h);
		hud_add_line(out, x + 0.12f*w, y + 0.05f*h, x + 0.86f*w, y + 0.05f*h);
		hud_add_line(out, x + 0.86f*w, y + 0.05f*h, x + 0.86f*w, y + 0.50f*h);
		hud_add_line(out, x + 0.12f*w, y + 0.50f*h, x + 0.86f*w, y + 0.50f*h);
		break;
	case 'S':
		hud_add_segment(out, 0, x, y, w, h);
		hud_add_segment(out, 5, x, y, w, h);
		hud_add_segment(out, 6, x, y, w, h);
		hud_add_segment(out, 2, x, y, w, h);
		hud_add_segment(out, 3, x, y, w, h);
		break;
	case 'M':
		hud_add_line(out, x + 0.12f*w, y + 0.95f*h, x + 0.12f*w, y + 0.05f*h);
		hud_add_line(out, x + 0.88f*w, y + 0.95f*h, x + 0.88f*w, y + 0.05f*h);
		hud_add_line(out, x + 0.12f*w, y + 0.05f*h, x + 0.50f*w, y + 0.52f*h);
		hud_add_line(out, x + 0.88f*w, y + 0.05f*h, x + 0.50f*w, y + 0.52f*h);
		break;
	case 'W':
		hud_add_line(out, x + 0.12f*w, y + 0.05f*h, x + 0.24f*w, y + 0.95f*h);
		hud_add_line(out, x + 0.24f*w, y + 0.95f*h, x + 0.50f*w, y + 0.40f*h);
		hud_add_line(out, x + 0.50f*w, y + 0.40f*h, x + 0.76f*w, y + 0.95f*h);
		hud_add_line(out, x + 0.76f*w, y + 0.95f*h, x + 0.88f*w, y + 0.05f*h);
		break;
	case 'N':
		hud_add_line(out, x + 0.12f*w, y + 0.95f*h, x + 0.12f*w, y + 0.05f*h);
		hud_add_line(out, x + 0.88f*w, y + 0.95f*h, x + 0.88f*w, y + 0.05f*h);
		hud_add_line(out, x + 0.12f*w, y + 0.05f*h, x + 0.88f*w, y + 0.95f*h);
		break;
	case 'T':
		hud_add_line(out, x + 0.10f*w, y + 0.05f*h, x + 0.90f*w, y + 0.05f*h);
		hud_add_line(out, x + 0.50f*w, y + 0.05f*h, x + 0.50f*w, y + 0.95f*h);
		break;
	case 'C':
		hud_add_line(out, x + 0.88f*w, y + 0.05f*h, x + 0.18f*w, y + 0.05f*h);
		hud_add_line(out, x + 0.18f*w, y + 0.05f*h, x + 0.18f*w, y + 0.95f*h);
		hud_add_line(out, x + 0.18f*w, y + 0.95f*h, x + 0.88f*w, y + 0.95f*h);
		break;
	case 'I':
		hud_add_line(out, x + 0.10f*w, y + 0.05f*h, x + 0.90f*w, y + 0.05f*h);
		hud_add_line(out, x + 0.50f*w, y + 0.05f*h, x + 0.50f*w, y + 0.95f*h);
		hud_add_line(out, x + 0.10f*w, y + 0.95f*h, x + 0.90f*w, y + 0.95f*h);
		break;
	case 'X':
		hud_add_line(out, x + 0.12f*w, y + 0.05f*h, x + 0.88f*w, y + 0.95f*h);
		hud_add_line(out, x + 0.88f*w, y + 0.05f*h, x + 0.12f*w, y + 0.95f*h);
		break;
	case 'O':
		hud_add_line(out, x + 0.18f*w, y + 0.05f*h, x + 0.82f*w, y + 0.05f*h);
		hud_add_line(out, x + 0.82f*w, y + 0.05f*h, x + 0.82f*w, y + 0.95f*h);
		hud_add_line(out, x + 0.82f*w, y + 0.95f*h, x + 0.18f*w, y + 0.95f*h);
		hud_add_line(out, x + 0.18f*w, y + 0.95f*h, x + 0.18f*w, y + 0.05f*h);
		break;
	case 'E':
		hud_add_line(out, x + 0.12f*w, y + 0.05f*h, x + 0.12f*w, y + 0.95f*h);
		hud_add_line(out, x + 0.12f*w, y + 0.05f*h, x + 0.88f*w, y + 0.05f*h);
		hud_add_line(out, x + 0.12f*w, y + 0.50f*h, x + 0.72f*w, y + 0.50f*h);
		hud_add_line(out, x + 0.12f*w, y + 0.95f*h, x + 0.88f*w, y + 0.95f*h);
		break;
	case ':':
		hud_add_line(out, x + 0.50f*w, y + 0.30f*h, x + 0.50f*w, y + 0.35f*h);
		hud_add_line(out, x + 0.50f*w, y + 0.65f*h, x + 0.50f*w, y + 0.70f*h);
		break;
	case '.':
		hud_add_line(out, x + 0.50f*w, y + 0.88f*h, x + 0.50f*w, y + 0.93f*h);
		break;
	case '-':
		hud_add_segment(out, 6, x, y, w, h);
		break;
	default:
		break;
	}
}

static void hud_add_text(vector<GLfloat> &out, const string &text, float x, float y, float char_w, float char_h)
{
	float pen_x = x;
	for (size_t i = 0; i < text.size(); i++)
	{
		char ch = text[i];
		if (ch == ' ')
		{
			pen_x += char_w * 0.7f;
			continue;
		}

		hud_add_char(out, ch, pen_x, y, char_w, char_h);
		pen_x += char_w;
	}
}

static void draw_hud()
{
	const glm::mat4 identity4(1.0f);
	const glm::mat3 identity3(1.0f);
	glUniformMatrix4fv(glGetUniformLocation(shaderprogram, "myprojection_matrix"), 1, GL_FALSE, &identity4[0][0]);
	glUniformMatrix4fv(glGetUniformLocation(shaderprogram, "myview_matrix"), 1, GL_FALSE, &identity4[0][0]);
	glUniformMatrix3fv(glGetUniformLocation(shaderprogram, "mynormal_matrix"), 1, GL_FALSE, &identity3[0][0]);

	const float panel_x = 12.0f;
	const float panel_y = 12.0f;
	const float panel_w = 250.0f;
	const float panel_h = 88.0f;

	vector<GLfloat> panel;
	hud_add_triangle(panel, panel_x, panel_y, panel_x + panel_w, panel_y, panel_x + panel_w, panel_y + panel_h);
	hud_add_triangle(panel, panel_x, panel_y, panel_x + panel_w, panel_y + panel_h, panel_x, panel_y + panel_h);

	vector<GLfloat> text_lines;
	const float char_w = 10.0f;
	const float char_h = 14.0f;
	hud_add_text(text_lines, "V: " + to_string(static_cast<long long>(m->vertices.size())), panel_x + 12.0f, panel_y + 16.0f, char_w, char_h);
	hud_add_text(text_lines, "H: " + to_string(static_cast<long long>(m->halfedges.size())), panel_x + 12.0f, panel_y + 36.0f, char_w, char_h);
	hud_add_text(text_lines, "F: " + to_string(static_cast<long long>(m->faces.size())), panel_x + 12.0f, panel_y + 56.0f, char_w, char_h);
	hud_add_text(text_lines, "FPS: " + to_string(static_cast<long long>(fps)), panel_x + 126.0f, panel_y + 56.0f, char_w, char_h);

	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	draw_overlay_geometry(GL_TRIANGLES, panel, { 1.0f, 1.0f, 1.0f, 0.72f });
	draw_overlay_geometry(GL_LINES, text_lines, { 0.08f, 0.08f, 0.08f, 1.0f }, 1.2f);

	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
}

static void draw_imgui_menu()
{
	extern bool g_show_imgui_menu;
	extern float g_imgui_menu_x, g_imgui_menu_y;

	if (!g_show_imgui_menu) return;

	ImGui::SetNextWindowPos(ImVec2(g_imgui_menu_x, g_imgui_menu_y), ImGuiCond_Appearing);
	ImGui::Begin("##ContextMenu", &g_show_imgui_menu,
		ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize);

	// Draw submenu
	if (ImGui::BeginMenu("Draw"))
	{
		if (ImGui::MenuItem("Vertex-shading/face-shading")) { menu(MENU_SHADINGTYPE); g_show_imgui_menu = false; }
		if (ImGui::MenuItem("Mesh")) { menu(MENU_DRAWMESH); g_show_imgui_menu = false; }
		if (ImGui::MenuItem("Wireframe")) { menu(MENU_DRAWWIREFRAME); g_show_imgui_menu = false; }
		if (ImGui::MenuItem("Vertices")) { menu(MENU_DRAWMESHVERTICES); g_show_imgui_menu = false; }
		if (ImGui::MenuItem("Normals")) { menu(MENU_DRAWNORMALS); g_show_imgui_menu = false; }
		if (ImGui::MenuItem("Silhouette")) { menu(MENU_DRAWSILHOUETTE); g_show_imgui_menu = false; }
		if (ImGui::MenuItem("Crease")) { menu(MENU_DRAWCREASE); g_show_imgui_menu = false; }
		ImGui::EndMenu();
	}

	// Mesh Operations submenu
	if (ImGui::BeginMenu("Mesh Operations"))
	{
		if (ImGui::MenuItem("Catmull-Clark subdivision")) { menu(MENU_CATMULLCLARK); g_show_imgui_menu = false; }
		if (ImGui::MenuItem("Loop subdivision")) { menu(MENU_LOOP); g_show_imgui_menu = false; }
		if (ImGui::MenuItem("Inflate")) { menu(MENU_INFLATE); g_show_imgui_menu = false; }
		if (ImGui::MenuItem("Smoothen")) { menu(MENU_SMOOTHEN); g_show_imgui_menu = false; }
		if (ImGui::MenuItem("Simplification")) { menu(MENU_SIMPLIFY); g_show_imgui_menu = false; }
		ImGui::EndMenu();
	}

	// Select submenu
	if (ImGui::BeginMenu("Select"))
	{
		if (ImGui::MenuItem("Closest Edge")) { menu(MENU_SELECTEDGE); g_show_imgui_menu = false; }
		if (ImGui::MenuItem("Closest Face")) { menu(MENU_SELECTFACE); g_show_imgui_menu = false; }
		if (ImGui::MenuItem("Closest Vertex")) { menu(MENU_SELECTVERTEX); g_show_imgui_menu = false; }
		if (ImGui::MenuItem("Clear")) { menu(MENU_SELECTCLEAR); g_show_imgui_menu = false; }
		ImGui::EndMenu();
	}

	// Face Operations submenu
	if (ImGui::BeginMenu("Face Operations"))
	{
		if (ImGui::MenuItem("Split edge")) { menu(MENU_SPLITEDGE); g_show_imgui_menu = false; }
		if (ImGui::MenuItem("Split face")) { menu(MENU_SPLITFACE); g_show_imgui_menu = false; }
		if (ImGui::MenuItem("Contract edge")) { menu(MENU_CONTRACTEDGE); g_show_imgui_menu = false; }
		if (ImGui::MenuItem("Contract face")) { menu(MENU_CONTRACTFACE); g_show_imgui_menu = false; }
		ImGui::EndMenu();
	}

	ImGui::Separator();

	if (ImGui::MenuItem("Open File")) { menu(MENU_OPENFILE); g_show_imgui_menu = false; }
	if (ImGui::MenuItem("Triangulate")) { menu(MENU_TRIANGULATE); g_show_imgui_menu = false; }
	if (ImGui::MenuItem("Write to File")) { menu(MENU_WRITE); g_show_imgui_menu = false; }
	if (ImGui::MenuItem("Undo")) { menu(MENU_UNDO); g_show_imgui_menu = false; }
	if (ImGui::MenuItem("Generate Mesh")) { menu(MENU_GENERATE); g_show_imgui_menu = false; }
	if (ImGui::MenuItem("Cut Mesh")) { menu(MENU_CUT); g_show_imgui_menu = false; }
	if (ImGui::MenuItem("Exit")) { menu(MENU_EXIT); g_show_imgui_menu = false; }

	ImGui::End();
}

void menu(int item)
{
	switch(item)
	{
	case MENU_TRIANGULATE:
		{
			m->triangulate();
			m->computeNormals();
			makeBuffers(m);
			break;
		}
	case MENU_SHADINGTYPE:
		{
			smooth = !smooth;
			break;
		}
	case MENU_DRAWMESH:
		{
			drawmesh = !drawmesh;
			break;
		}
	case MENU_DRAWMESHVERTICES:
		{
			drawmeshvertices = !drawmeshvertices;
			break;
		}
	case MENU_DRAWWIREFRAME:
		{
			drawwireframe = !drawwireframe;
			break;
		}
	case MENU_DRAWNORMALS:
		{
			drawnormals = !drawnormals;
			break;
		}
	case MENU_DRAWSILHOUETTE:
		{
			drawsilhouette = !drawsilhouette;
			break;
		}
	case MENU_SELECTCLEAR:
		{
			clear();
			break;
		}
	case MENU_SELECTEDGE:
		{
			if (pickedpoint == NULL || m->halfedges.empty()) break;
			closest_edge = (*m->halfedges.begin());
			double min = std::numeric_limits<double>::max();
			for (vector<myHalfedge *>::iterator it = m->halfedges.begin(); it != m->halfedges.end(); it++)
			{
				myHalfedge *e = (*it);
				myVertex *v1 = (*it)->source;
				if ((*it)->twin == NULL) continue;
				myVertex *v2 = (*it)->twin->source;

				double d = pickedpoint->dist(v1->point, v2->point);
				if (d < min) { min = d; closest_edge = e; } 
			}
			break;
		}
	case MENU_SELECTVERTEX:
		{
			if (pickedpoint == NULL || m->vertices.empty()) break;
			closest_vertex = (*m->vertices.begin());
			double min = std::numeric_limits<double>::max();
			for (vector<myVertex *>::iterator it = m->vertices.begin(); it != m->vertices.end(); it++)
			{
				double d = pickedpoint->dist(*( (*it)->point ));
				if (d<min) { min = d; closest_vertex = *it; }
			}
			break;
		}
	case MENU_INFLATE:
		{
			for (vector<myVertex *>::iterator it = m->vertices.begin(); it != m->vertices.end(); it++)
			{
				myVector3D delta = *((*it)->normal) * 0.01;
				*((*it)->point) = *((*it)->point) + delta;
			}
			makeBuffers(m);
			break;
	}
	case MENU_CATMULLCLARK:
		{
			m->subdivisionCatmullClark();
			clear();
			m->computeNormals();
			makeBuffers(m);
			break;
		}
	case MENU_SPLITEDGE:
		{
			if (pickedpoint != NULL && closest_edge != NULL)	
				m->splitEdge(closest_edge, pickedpoint);
			clear();
			m->computeNormals();
			makeBuffers(m);
			break;
		}		
	case MENU_SPLITFACE:
		{
			if (pickedpoint != NULL && closest_face != NULL)	
				m->splitFaceTRIS(closest_face, pickedpoint);
			clear();		
			m->computeNormals();
			makeBuffers(m);
			break;
		}
	case MENU_OPENFILE:
	    {
#if defined(MESHVIEWER_ENABLE_NFD)
			NFD::Guard nfdGuard;
			NFD::UniquePath outPath;
			nfdfilteritem_t filters[1] = { { "Wavefront OBJ", "obj" } };
			nfdresult_t result = NFD::OpenDialog(outPath, filters, 1);
			if (result == NFD_OKAY)
			{
				m->clear();
				if (m->readFile(outPath.get()))
				{
					m->computeNormals();
					makeBuffers(m);
				}
			}
#endif
			break;
	    }
	case MENU_EXIT:
		{
			g_running = false;
			break;
		}
	}
}




//This function is called to display objects on screen.
void display() 
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glViewport(0, 0, window_w, window_h);
	glUseProgram(shaderprogram);

	glm::mat4 projection_matrix = glm::perspective(glm::radians(fovy), (float)window_w / (float)window_h, zNear, zFar);
	glUniformMatrix4fv(glGetUniformLocation(shaderprogram, "myprojection_matrix"),
		1, GL_FALSE, &projection_matrix[0][0]);

	glm::mat4 view_matrix = glm::lookAt(glm::vec3(camera_eye.X, camera_eye.Y, camera_eye.Z),
										glm::vec3(camera_eye.X + camera_forward.dX, camera_eye.Y + camera_forward.dY, camera_eye.Z + camera_forward.dZ),
										glm::vec3(camera_up.dX, camera_up.dY, camera_up.dZ));
	glUniformMatrix4fv(glGetUniformLocation(shaderprogram, "myview_matrix"),
		1, GL_FALSE, &view_matrix[0][0]);

	glm::mat3 normal_matrix = glm::transpose(glm::inverse(glm::mat3(view_matrix)));
	glUniformMatrix3fv(glGetUniformLocation(shaderprogram, "mynormal_matrix"),
		1, GL_FALSE, &normal_matrix[0][0]);

	glUniform1i(glGetUniformLocation(shaderprogram, "type"), 0);
 
	array<GLfloat, 4> color = { 0.0f, 0.0f, 0.0f, 1.0f };

	if ( (drawmesh && vaos[VAO_TRIANGLES_NORMSPERVERTEX]) || drawsilhouette)
	{
		glLineWidth(1.0);
		glEnable(GL_POLYGON_OFFSET_FILL); glPolygonOffset(2.0f, 2.0f); //for z-bleeding, z-fighting.
		if (drawsilhouette && !drawmesh)  glUniform1i(glGetUniformLocation(shaderprogram, "type"), 1);
		if (drawmesh) color = { 0.4f, 0.8f, 0.4f, 1.0f };
		else color = { 1.0f, 1.0f, 1.0f, 1.0f };
		glUniform4fv(glGetUniformLocation(shaderprogram, "kd"), 1, color.data());

		if (smooth)
		{
			glBindVertexArray(vaos[VAO_TRIANGLES_NORMSPERVERTEX]);
			glDrawArrays(GL_TRIANGLES, 0, num_triangles * 3);
			glBindVertexArray(0);
		}
		else 
		{
			glBindVertexArray(vaos[VAO_TRIANGLES_NORMSPERFACE]);
			glDrawArrays(GL_TRIANGLES, 0, num_triangles * 3);
			glBindVertexArray(0);
		}

		glUniform1i(glGetUniformLocation(shaderprogram, "type"), 0);
		glDisable(GL_POLYGON_OFFSET_FILL);
	}

	if (drawmeshvertices && vaos[VAO_VERTICES])
	{
		glPointSize(4.0);
		color = { 0.0f, 0.0f, 0.0f, 1.0f };
		glUniform4fv(glGetUniformLocation(shaderprogram, "kd"), 1, color.data());

		glBindVertexArray(vaos[VAO_VERTICES]);
		glDrawElements(GL_POINTS, m->vertices.size(), GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);
	}

	if (drawwireframe && vaos[VAO_EDGES])
	{
		glLineWidth(2.0);
		color = { 0.0f, 0.0f, 0.0f, 1.0f };
		glUniform4fv(glGetUniformLocation(shaderprogram, "kd"), 1, color.data());
		glBindVertexArray(vaos[VAO_EDGES]);
		glDrawElements(GL_LINES, m->halfedges.size()*2, GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);
	}

	if (drawsilhouette)
	{
		vector<GLfloat> silhouette_vertices;
		for (vector<myHalfedge *>::iterator it = m->halfedges.begin(); it != m->halfedges.end(); it++)
		{
			myVertex *v1 = (*it)->source;
			if ((*it)->twin == NULL) continue;
			myVertex *v2 = (*it)->twin->source;

			myFace *f1 = (*it)->adjacent_face;
			myFace *f2 = (*it)->twin->adjacent_face;
			if (f1 == NULL || f2 == NULL) continue;
			if (f1->normal == NULL || f2->normal == NULL) continue;

			myPoint3D mid((v1->point->X + v2->point->X) / 2.0,
			              (v1->point->Y + v2->point->Y) / 2.0,
			              (v1->point->Z + v2->point->Z) / 2.0);
						  
			myVector3D view(camera_eye.X - mid.X,
			                camera_eye.Y - mid.Y,
			                camera_eye.Z - mid.Z);

			double dot1 = f1->normal->dX * view.dX + f1->normal->dY * view.dY + f1->normal->dZ * view.dZ;
			double dot2 = f2->normal->dX * view.dX + f2->normal->dY * view.dY + f2->normal->dZ * view.dZ;

			if (dot1 * dot2 < 0)
			{
				append_overlay_vertex(silhouette_vertices, v1->point);
				append_overlay_vertex(silhouette_vertices, v2->point);
			}				
		}
		draw_overlay_geometry(GL_LINES, silhouette_vertices, { 1.0f, 0.0f, 0.0f, 1.0f }, 4.0f);
 	}

	if (drawnormals && vaos[VAO_NORMALS])
	{
		glLineWidth(1.0);
		color = { 0.2f, 0.2f, 0.2f, 1.0f };
		glUniform4fv(glGetUniformLocation(shaderprogram, "kd"), 1, color.data());

		glBindVertexArray(vaos[VAO_NORMALS]);
		glDrawArrays(GL_LINES, 0, m->vertices.size()*2 );
		glBindVertexArray(0);
	}

	if (pickedpoint != NULL)
	{
		vector<GLfloat> picked_vertex;
		append_overlay_vertex(picked_vertex, pickedpoint);
		draw_overlay_geometry(GL_POINTS, picked_vertex, { 1.0f, 0.0f, 1.0f, 1.0f }, 1.0f, 8.0f);
	}

	if (closest_edge != NULL)
	{
		vector<GLfloat> edge_vertices;
		append_overlay_vertex(edge_vertices, closest_edge->source ? closest_edge->source->point : NULL);
		append_overlay_vertex(edge_vertices, (closest_edge->twin && closest_edge->twin->source) ? closest_edge->twin->source->point : NULL);
		draw_overlay_geometry(GL_LINES, edge_vertices, { 1.0f, 0.0f, 1.0f, 1.0f }, 4.0f);
	}

	if (closest_vertex != NULL)
	{
		vector<GLfloat> vertex_marker;
		append_overlay_vertex(vertex_marker, closest_vertex->point);
		draw_overlay_geometry(GL_POINTS, vertex_marker, { 0.0f, 0.0f, 0.0f, 1.0f }, 1.0f, 8.0f);
	}

	if (closest_face != NULL)
	{
		vector<GLfloat> face_tris = build_face_overlay_triangles(closest_face);
		draw_overlay_geometry(GL_TRIANGLES, face_tris, { 0.1f, 0.1f, 0.9f, 1.0f });
	}

	draw_hud();

	// Start ImGui frame
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplSDL2_NewFrame();
	ImGui::NewFrame();

	// Draw ImGui menu
	draw_imgui_menu();

	// Render ImGui
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

	if (g_window != NULL) SDL_GL_SwapWindow(g_window);
}


void initMesh()
{
	pickedpoint = NULL;
	closest_edge = NULL;
	closest_vertex = NULL;
	closest_face = NULL;

	m = new myMesh();
	if (m->readFile(resolve_resource_path("gear.obj"))) {
		m->computeNormals();
		makeBuffers(m);
	}

}


int main(int argc, char* argv[]) 
{
	initInterface(argc, argv);

	initMesh();

	runMainLoop();

	if (pickedpoint != NULL)
	{
		delete pickedpoint;
		pickedpoint = NULL;
	}

	if (m != NULL)
	{
		m->clear();
		delete m;
		m = NULL;
	}

	shutdownInterface();
	return 0;
}
