#include "level.hpp"

#include "resource.hpp"

level::level(int xs, int ys)
	: m_tmap(resource::get().tex("assets/tiles.png"), xs, ys, 16) {
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
	m_metadata = level::metadata(data);
	map().load(data.code);
}

tilemap& level::map() {
	return m_tmap;
}

const tilemap& level::map() const {
	return m_tmap;
}

void level::clear() {
	m_metadata.reset();
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

level::metadata::metadata(api::level data)
	: id(data.id),
	  author(data.author),
	  title(data.title),
	  description(data.description),
	  createdAt(data.createdAt),
	  updatedAt(data.updatedAt),
	  downloads(data.downloads) {
}
