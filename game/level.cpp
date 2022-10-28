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

void level::load_from_api(api::level data) {
	m_metadata			  = level::metadata(data.id, data.author, data.title, data.description);
	m_metadata->createdAt = data.createdAt;
	m_metadata->updatedAt = data.updatedAt;
	map().load(data.code);
}

tilemap& level::map() {
	return m_tmap;
}

const tilemap& level::map() const {
	return m_tmap;
}

void level::clear() {
	m_metadata = {};
	map().clear();
}

bool level::has_metadata() const {
	return !!m_metadata;
}

level::metadata& level::get_metadata() {
	return *m_metadata;
}

const level::metadata& level::get_metadata() const {
	return *m_metadata;
}

void level::clear_metadata() {
	m_metadata.reset();
}

level::metadata::metadata(int id, std::string author, std::string title, std::string description)
	: id(id),
	  author(author),
	  title(title),
	  description(description) {
}
