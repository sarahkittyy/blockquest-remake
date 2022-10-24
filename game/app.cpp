#include "app.hpp"
#include "imgui-SFML.h"

#include <fstream>
#include <iostream>

#include "debug.hpp"

#include "states/debug.hpp"
#include "states/edit.hpp"
#include "states/main.hpp"

app::app(int argc, char** argv)
	: m_window(sf::VideoMode(1920, 1048), "BlockQuest Remake"),
	  m_r(m_window),
	  m_fsm(m_r) {
	if (!ImGui::SFML::Init(m_window))
		throw "Could not initialize ImGui";

	sf::Image icon;
	icon.loadFromFile("assets/gui/play.png");
	m_window.setIcon(icon.getSize().x, icon.getSize().y, icon.getPixelsPtr());

	m_r.load_sound("jump", "assets/sound/jump.flac");
	m_r.load_sound("dash", "assets/sound/dash.flac");
	m_r.load_sound("gameover", "assets/sound/gameover.flac");
	m_r.load_sound("gravityflip", "assets/sound/gravityflip.flac");
	m_r.load_sound("wallkick", "assets/sound/wallkick.flac");
	m_r.load_sound("victory", "assets/sound/victory.flac");

	configure_imgui_style();

	if (argc > 1) {
		std::string mode = argv[1];
		if (mode == "main") {
			m_fsm.swap_state<states::main>();
		} else if (mode == "debug") {
			m_fsm.swap_state<states::debug>();
		} else if (mode == "edit") {
			m_fsm.swap_state<states::edit>();
		} else {
			m_fsm.swap_state<states::edit>();
		}
	} else {
		m_fsm.swap_state<states::edit>();
	}
}

int app::run() {
	sf::Clock delta_clock;	 // clock for measuring time deltas between frames
	sf::Event evt;

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
			case sf::Event::Resized:
				sf::FloatRect visibleArea(0, 0, evt.size.width, evt.size.height);
				m_window.setView(sf::View(visibleArea));
				break;
			}
			m_fsm.process_event(evt);
		}
		// updates
		sf::Time dt = delta_clock.restart();
		ImGui::SFML::Update(m_window, dt);
		debug::get().flush();
		m_fsm.update(dt);

		// imgui rendering
		m_fsm.imdraw();
		debug::get().imdraw(dt);
		ImGui::EndFrame();

		m_window.clear(m_fsm.bg());

		// draw
		m_window.draw(m_fsm);
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
	style.WindowRounding  = 12;
	style.TabBorderSize	  = 1;
	style.FramePadding	  = ImVec2(18, 7);

	// font
	ImGuiIO& io = ImGui::GetIO();
	io.Fonts->AddFontDefault();
	io.Fonts->AddFontFromFileTTF("assets/verdana.ttf", 14);
	io.Fonts->Build();
	if (!ImGui::SFML::UpdateFontTexture())
		throw "Could not update ImGui::SFML font texture";

	// color
	ImVec4* colors						   = ImGui::GetStyle().Colors;
	colors[ImGuiCol_Text]				   = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
	colors[ImGuiCol_TextDisabled]		   = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
	colors[ImGuiCol_WindowBg]			   = ImVec4(0.00f, 0.00f, 0.00f, 0.88f);
	colors[ImGuiCol_ChildBg]			   = ImVec4(0.16f, 0.10f, 0.00f, 1.00f);
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

int main(int argc, char** argv) {
	app app(argc, argv);
	return app.run();
}
