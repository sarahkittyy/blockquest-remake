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
	w.setScale(2.0f, 2.0f);
	debug::get().setScale(2.0f, 2.0f);

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
