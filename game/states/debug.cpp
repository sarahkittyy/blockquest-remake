#include "debug.hpp"

#include <fstream>

namespace states {

debug::debug(resource& r)
	: state(r) {
	level l(r);
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

	m_w = std::make_unique<world>(r, l);
	// w.setScale(2.0f, 2.0f);
	// debug::get().setScale(2.0f, 2.0f);
}

debug::~debug() {
}

void debug::update(fsm* sm, sf::Time dt) {
	m_w->update(dt);
}

void debug::draw(sf::RenderTarget& t, sf::RenderStates s) const {
	s.transform *= getTransform();
	t.draw(*m_w, s);
}

}
