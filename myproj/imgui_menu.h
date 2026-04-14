#pragma once
#include <imgui.h>

// Simple wrapper to show the right-click context menu using ImGui
static bool g_show_context_menu = false;
static ImVec2 g_context_menu_pos;

inline void open_context_menu_at(float x, float y)
{
	g_show_context_menu = true;
	g_context_menu_pos = ImVec2(x, y);
	ImGui::SetNextWindowPos(g_context_menu_pos, ImGuiCond_Appearing);
}

inline void close_context_menu()
{
	g_show_context_menu = false;
}

inline bool is_context_menu_open()
{
	return g_show_context_menu;
}

// Draw the menu - call this every frame from your render loop
inline void draw_context_menu(void (*menu_callback)(int))
{
	if (!g_show_context_menu) return;

	ImGui::SetNextWindowPos(g_context_menu_pos, ImGuiCond_Appearing);
	if (ImGui::Begin("##ContextMenu", &g_show_context_menu,
		ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize))
	{
		// Draw submenu
		if (ImGui::BeginMenu("Draw"))
		{
			if (ImGui::MenuItem("Vertex-shading/face-shading")) { menu_callback(0 /* MENU_SHADINGTYPE */); close_context_menu(); }
			if (ImGui::MenuItem("Mesh")) { menu_callback(3 /* MENU_DRAWMESH */); close_context_menu(); }
			if (ImGui::MenuItem("Wireframe")) { menu_callback(1 /* MENU_DRAWWIREFRAME */); close_context_menu(); }
			if (ImGui::MenuItem("Vertices")) { menu_callback(5 /* MENU_DRAWMESHVERTICES */); close_context_menu(); }
			if (ImGui::MenuItem("Normals")) { menu_callback(25 /* MENU_DRAWNORMALS */); close_context_menu(); }
			if (ImGui::MenuItem("Silhouette")) { menu_callback(9 /* MENU_DRAWSILHOUETTE */); close_context_menu(); }
			if (ImGui::MenuItem("Crease")) { menu_callback(8 /* MENU_DRAWCREASE */); close_context_menu(); }
			ImGui::EndMenu();
		}

		// Mesh Operations submenu
		if (ImGui::BeginMenu("Mesh Operations"))
		{
			if (ImGui::MenuItem("Catmull-Clark subdivision")) { menu_callback(0 /* MENU_CATMULLCLARK */); close_context_menu(); }
			if (ImGui::MenuItem("Loop subdivision")) { menu_callback(4 /* MENU_LOOP */); close_context_menu(); }
			if (ImGui::MenuItem("Inflate")) { menu_callback(12 /* MENU_INFLATE */); close_context_menu(); }
			if (ImGui::MenuItem("Smoothen")) { menu_callback(17 /* MENU_SMOOTHEN */); close_context_menu(); }
			if (ImGui::MenuItem("Simplification")) { menu_callback(24 /* MENU_SIMPLIFY */); close_context_menu(); }
			ImGui::EndMenu();
		}

		// Select submenu
		if (ImGui::BeginMenu("Select"))
		{
			if (ImGui::MenuItem("Closest Edge")) { menu_callback(13 /* MENU_SELECTEDGE */); close_context_menu(); }
			if (ImGui::MenuItem("Closest Face")) { menu_callback(14 /* MENU_SELECTFACE */); close_context_menu(); }
			if (ImGui::MenuItem("Closest Vertex")) { menu_callback(15 /* MENU_SELECTVERTEX */); close_context_menu(); }
			if (ImGui::MenuItem("Clear")) { menu_callback(20 /* MENU_SELECTCLEAR */); close_context_menu(); }
			ImGui::EndMenu();
		}

		// Face Operations submenu
		if (ImGui::BeginMenu("Face Operations"))
		{
			if (ImGui::MenuItem("Split edge")) { menu_callback(18 /* MENU_SPLITEDGE */); close_context_menu(); }
			if (ImGui::MenuItem("Split face")) { menu_callback(19 /* MENU_SPLITFACE */); close_context_menu(); }
			if (ImGui::MenuItem("Contract edge")) { menu_callback(6 /* MENU_CONTRACTEDGE */); close_context_menu(); }
			if (ImGui::MenuItem("Contract face")) { menu_callback(7 /* MENU_CONTRACTFACE */); close_context_menu(); }
			ImGui::EndMenu();
		}

		ImGui::Separator();

#if defined(MESHVIEWER_ENABLE_NFD)
		if (ImGui::MenuItem("Open File")) { menu_callback(26 /* MENU_OPENFILE */); close_context_menu(); }
#endif
		if (ImGui::MenuItem("Triangulate")) { menu_callback(21 /* MENU_TRIANGULATE */); close_context_menu(); }
		if (ImGui::MenuItem("Write to File")) { menu_callback(23 /* MENU_WRITE */); close_context_menu(); }
		if (ImGui::MenuItem("Undo")) { menu_callback(22 /* MENU_UNDO */); close_context_menu(); }
		if (ImGui::MenuItem("Generate Mesh")) { menu_callback(10 /* MENU_GENERATE */); close_context_menu(); }
		if (ImGui::MenuItem("Cut Mesh")) { menu_callback(11 /* MENU_CUT */); close_context_menu(); }
		if (ImGui::MenuItem("Exit")) { menu_callback(2 /* MENU_EXIT */); close_context_menu(); }

		ImGui::End();
	}
	else
	{
		close_context_menu();
	}
}
