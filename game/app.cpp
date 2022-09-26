#include "app.hpp"
#include "imgui-SFML.h"

#include <fstream>
#include <iostream>

#include "debug.hpp"

app::app()
	: m_window(sf::VideoMode(1366, 768), "BlockQuest Remake") {
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
	for (int i = 0; i < 16; ++i) {
		l.map().set(i, 15, tile::block);
		l.map().set(i, 0, tile::block);
	}
	l.map().set(0, 1, tile(tile::block, { .moving = 3 }));
	l.map().set(2, 14, tile::begin);
	l.map().set(1, 13, tile::block);

	// l.map().set(1, 14, tile(tile::block, { .moving = 2 }));
	l.map().set(0, 14, tile(tile::block, { .moving = 2 }));
	l.map().set(15, 13, tile(tile::gravity, { .moving = 4 }));

	l.map().set(14, 14, tile::end);

	l.map().set_editor_view(false);

	world w(m_r, l);
	w.setScale(2.0f, 2.0f);
	debug::get().setScale(2.0f, 2.0f);

	std::ofstream file("misc/moving_test.json");
	file << l.serialize();
	file.close();

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

int main() {
	app app;
	return app.run();
}
