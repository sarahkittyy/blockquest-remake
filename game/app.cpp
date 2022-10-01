#include "app.hpp"
#include "imgui-SFML.h"

#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

#include "debug.hpp"

app::app()
	: m_window(sf::VideoMode(1920, 1080), "BlockQuest Remake") {
	ImGui::SFML::Init(m_window);
}

int app::run() {
	sf::Clock delta_clock;	 // clock for measuring time deltas between frames
	sf::Event evt;

	m_r.load_sound("jump", "assets/sound/jump.flac");
	m_r.load_sound("dash", "assets/sound/dash.flac");
	m_r.load_sound("gameover", "assets/sound/gameover.flac");
	m_r.load_sound("gravityflip", "assets/sound/gravityflip.flac");
	m_r.load_sound("wallkick", "assets/sound/wallkick.flac");

	level l(m_r);
	for (int i = 0; i < 32; ++i) {
		l.map().set(i, 31, tile::block);
		l.map().set(i, 0, tile::ice);
		l.map().set(0, i, tile::block);
		l.map().set(31, i, i < 15 ? tile::ladder : tile::black);
	}

	l.map().set(1, 30, tile::begin);
	l.map().set(1, 29, tile::block);
	l.map().set(1, 26, tile::spike);
	l.map().set(1, 2, tile::block);
	l.map().set(18, 30, tile::end);

	for (int i = 5; i < 29; ++i) {
		l.map().set(5, i, tile::ladder);
		if (i % 6 == 0 && i > 9) {
			l.map().set(4, i, tile::spike);
		}
	}

	l.map().set(6, 3, tile::block);

	l.map().set(16, 31, tile::gravity);
	l.map().set(16, 29, tile::block);
	l.map().set(16, 2, tile::block);
	l.map().set(16, 0, tile::gravity);
	l.map().set(17, 16, tile::spike);

	l.map().set(8, 30, tile::block);
	l.map().set(8, 29, tile::block);
	l.map().set(9, 28, tile::block);
	l.map().set(11, 27, tile::block);
	l.map().set(18, 27, tile(tile::ice, { .moving = 4 }));
	l.map().set(18, 26, tile(tile::ladder, { .moving = 4 }));
	l.map().set(18, 25, tile(tile::ladder, { .moving = 4 }));

	// l.map().set(8, 1, tile::block);
	// l.map().set(8, 2, tile::block);
	// l.map().set(9, 3, tile::block);
	// l.map().set(11, 4, tile::block);
	// l.map().set(18, 4, tile(tile::block, { .moving = 4 }));

	l.map().set(23, 30, tile::block);
	l.map().set(23, 28, tile::block);

	l.map().set(23, 1, tile::block);
	l.map().set(23, 3, tile::block);

	l.map().set_editor_view(false);

	std::ofstream file("misc/test_dash.json");
	file << l.serialize();
	file.close();

	world w(m_r, l);
	// w.setScale(2.0f, 2.0f);
	// debug::get().setScale(2.0f, 2.0f);

	configure_imgui_style();

	// app loop
	while (m_window.isOpen()) {
		while (m_window.pollEvent(evt)) {
			ImGui::SFML::ProcessEvent(evt);
			switch (evt.type) {
			default:
				break;
			case sf::Event::Closed:
				m_window.close();
				break;
			}
		}
		// updates
		sf::Time dt = delta_clock.restart();
		ImGui::SFML::Update(m_window, dt);
		debug::get().flush();
		w.update(dt);

		// imgui rendering
		debug::get().imdraw();
		ImGui::ShowDemoWindow();
		ImGui::EndFrame();

		m_window.clear(sf::Color(0xC8AD7FFF));

		// draw
		m_window.draw(w);
		m_window.draw(debug::get());

		ImGui::SFML::Render(m_window);
		m_window.display();
	}

	ImGui::SFML::Shutdown(m_window);

	return 0;
}

void app::configure_imgui_style() {
	ImGuiStyle& style	  = ImGui::GetStyle();
	style.FrameRounding	  = 6;
	style.FrameBorderSize = 1;


	// color
	ImVec4* colors						   = style.Colors;
	colors[ImGuiCol_Text]				   = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
	colors[ImGuiCol_TextDisabled]		   = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
	colors[ImGuiCol_WindowBg]			   = ImVec4(0.00f, 0.00f, 0.00f, 0.94f);
	colors[ImGuiCol_ChildBg]			   = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	colors[ImGuiCol_PopupBg]			   = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
	colors[ImGuiCol_Border]				   = ImVec4(0.75f, 0.52f, 0.16f, 1.00f);
	colors[ImGuiCol_BorderShadow]		   = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
	colors[ImGuiCol_FrameBg]			   = ImVec4(0.31f, 0.18f, 0.00f, 1.00f);
	colors[ImGuiCol_FrameBgHovered]		   = ImVec4(0.59f, 0.34f, 0.00f, 1.00f);
	colors[ImGuiCol_FrameBgActive]		   = ImVec4(0.64f, 0.37f, 0.01f, 1.00f);
	colors[ImGuiCol_TitleBg]			   = ImVec4(0.13f, 0.08f, 0.00f, 1.00f);
	colors[ImGuiCol_TitleBgActive]		   = ImVec4(0.34f, 0.20f, 0.00f, 1.00f);
	colors[ImGuiCol_TitleBgCollapsed]	   = ImVec4(0.16f, 0.09f, 0.00f, 1.00f);
	colors[ImGuiCol_MenuBarBg]			   = ImVec4(0.39f, 0.22f, 0.00f, 1.00f);
	colors[ImGuiCol_ScrollbarBg]		   = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
	colors[ImGuiCol_ScrollbarGrab]		   = ImVec4(0.44f, 0.25f, 0.00f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.31f, 0.18f, 0.00f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.31f, 0.18f, 0.00f, 1.00f);
	colors[ImGuiCol_CheckMark]			   = ImVec4(0.75f, 0.52f, 0.16f, 1.00f);
	colors[ImGuiCol_SliderGrab]			   = ImVec4(0.81f, 0.46f, 0.00f, 1.00f);
	colors[ImGuiCol_SliderGrabActive]	   = ImVec4(1.00f, 0.67f, 0.24f, 1.00f);
	colors[ImGuiCol_Button]				   = ImVec4(0.31f, 0.18f, 0.00f, 1.00f);
	colors[ImGuiCol_ButtonHovered]		   = ImVec4(0.59f, 0.34f, 0.00f, 1.00f);
	colors[ImGuiCol_ButtonActive]		   = ImVec4(0.59f, 0.34f, 0.00f, 1.00f);
	colors[ImGuiCol_Header]				   = ImVec4(0.31f, 0.18f, 0.00f, 1.00f);
	colors[ImGuiCol_HeaderHovered]		   = ImVec4(0.59f, 0.34f, 0.00f, 1.00f);
	colors[ImGuiCol_HeaderActive]		   = ImVec4(0.59f, 0.34f, 0.00f, 1.00f);
	colors[ImGuiCol_Separator]			   = ImVec4(0.31f, 0.18f, 0.00f, 1.00f);
	colors[ImGuiCol_SeparatorHovered]	   = ImVec4(0.59f, 0.34f, 0.00f, 1.00f);
	colors[ImGuiCol_SeparatorActive]	   = ImVec4(0.59f, 0.34f, 0.00f, 1.00f);
	colors[ImGuiCol_ResizeGrip]			   = ImVec4(0.26f, 0.59f, 0.98f, 0.20f);
	colors[ImGuiCol_ResizeGripHovered]	   = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
	colors[ImGuiCol_ResizeGripActive]	   = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
	colors[ImGuiCol_Tab]				   = ImVec4(0.31f, 0.18f, 0.00f, 1.00f);
	colors[ImGuiCol_TabHovered]			   = ImVec4(0.59f, 0.34f, 0.00f, 1.00f);
	colors[ImGuiCol_TabActive]			   = ImVec4(0.59f, 0.34f, 0.00f, 1.00f);
	colors[ImGuiCol_TabUnfocused]		   = ImVec4(0.14f, 0.08f, 0.00f, 1.00f);
	colors[ImGuiCol_TabUnfocusedActive]	   = ImVec4(0.23f, 0.13f, 0.00f, 1.00f);
	colors[ImGuiCol_PlotLines]			   = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
	colors[ImGuiCol_PlotLinesHovered]	   = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
	colors[ImGuiCol_PlotHistogram]		   = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
	colors[ImGuiCol_PlotHistogramHovered]  = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
	colors[ImGuiCol_TableHeaderBg]		   = ImVec4(0.59f, 0.34f, 0.00f, 1.00f);
	colors[ImGuiCol_TableBorderStrong]	   = ImVec4(0.39f, 0.22f, 0.00f, 1.00f);
	colors[ImGuiCol_TableBorderLight]	   = ImVec4(0.23f, 0.13f, 0.00f, 1.00f);
	colors[ImGuiCol_TableRowBg]			   = ImVec4(0.13f, 0.08f, 0.00f, 1.00f);
	colors[ImGuiCol_TableRowBgAlt]		   = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
	colors[ImGuiCol_TextSelectedBg]		   = ImVec4(0.76f, 0.43f, 0.00f, 1.00f);
	colors[ImGuiCol_DragDropTarget]		   = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
	colors[ImGuiCol_NavHighlight]		   = ImVec4(0.59f, 0.34f, 0.00f, 1.00f);
	colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
	colors[ImGuiCol_NavWindowingDimBg]	   = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
	colors[ImGuiCol_ModalWindowDimBg]	   = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
}

int main() {
	app app;
	return app.run();
}
