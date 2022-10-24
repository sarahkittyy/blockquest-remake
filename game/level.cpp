#include "level.hpp"

level::level(resource& r, int xs, int ys)
	: m_r(r),
	  m_tmap(m_r.tex("assets/tiles.png"), xs, ys, 16) {
}

void level::draw(sf::RenderTarget& t, sf::RenderStates s) const {
	s.transform *= getTransform();
	t.draw(m_tmap, s);
}

bool level::valid() const {
	return m_tmap.tile_count(tile::begin) == 1 &&
		   m_tmap.tile_count(tile::end) == 1;
}

sf::Vector2i level::mouse_tile(sf::Vector2f mouse_pos) const {
	return sf::Vector2i(
		mouse_pos.x / m_tmap.tile_size(),
		mouse_pos.y / m_tmap.tile_size());
}

tilemap& level::map() {
	return m_tmap;
}

const tilemap& level::map() const {
	return m_tmap;
}

