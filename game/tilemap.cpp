#include "tilemap.hpp"
#include <SFML/Graphics/RenderTarget.hpp>

#include "debug.hpp"
#include "util.hpp"

tilemap::tilemap(sf::Texture& tex, int xs, int ys, int ts)
	: m_tex(tex), m_xs(xs), m_ys(ys), m_ts(ts), m_editor(false) {
	m_va.setPrimitiveType(sf::Quads);
	m_va_editor.setPrimitiveType(sf::Quads);
	m_tiles.resize(xs * ys, tile::empty);
	m_flush_va();
}

void tilemap::draw(sf::RenderTarget& t, sf::RenderStates s) const {
	s.transform *= getTransform();
	s.texture = &m_tex;
	t.draw(m_va, s);
	if (m_editor) {
		t.draw(m_va_editor, s);
	}
}

void tilemap::m_flush_va() {
	m_va.clear();
	m_va.resize(m_xs * m_ys * 4);
	m_va_editor.clear();
	m_va_editor.resize(m_xs * m_ys * 4);
	for (int i = 0; i < m_tiles.size(); ++i) {
		m_update_quad(i);
	}
}

void tilemap::m_set_quad(int i, tile t) {
	// x and y of current tile
	int x = i % m_xs;
	int y = i / m_xs;

	if (t == tile::empty) {
		sf::Vertex air;
		m_va[i * 4]		= air;
		m_va[i * 4 + 1] = air;
		m_va[i * 4 + 2] = air;
		m_va[i * 4 + 3] = air;
		return;
	}

	int tx = int(t) % (m_tex.getSize().x / m_ts);
	int ty = int(t) / (m_tex.getSize().x / m_ts);

	// tiles only visible in editor mode
	sf::VertexArray& va_to_modify = m_va;
	if (t == tile::stopper) {
		va_to_modify = m_va_editor;
		return;
	}

	// fill in the quad
	va_to_modify[i * 4].position.x	= x * m_ts;
	va_to_modify[i * 4].position.y	= y * m_ts;
	va_to_modify[i * 4].texCoords.x = tx * m_ts;
	va_to_modify[i * 4].texCoords.y = ty * m_ts;

	va_to_modify[i * 4 + 1].position.x	= (x + 1) * m_ts;
	va_to_modify[i * 4 + 1].position.y	= y * m_ts;
	va_to_modify[i * 4 + 1].texCoords.x = (tx + 1) * m_ts;
	va_to_modify[i * 4 + 1].texCoords.y = ty * m_ts;

	va_to_modify[i * 4 + 2].position.x	= (x + 1) * m_ts;
	va_to_modify[i * 4 + 2].position.y	= (y + 1) * m_ts;
	va_to_modify[i * 4 + 2].texCoords.x = (tx + 1) * m_ts;
	va_to_modify[i * 4 + 2].texCoords.y = (ty + 1) * m_ts;

	va_to_modify[i * 4 + 3].position.x	= x * m_ts;
	va_to_modify[i * 4 + 3].position.y	= (y + 1) * m_ts;
	va_to_modify[i * 4 + 3].texCoords.x = tx * m_ts;
	va_to_modify[i * 4 + 3].texCoords.y = (ty + 1) * m_ts;

	// render movement arrows in editor mode
	if (t.props.moving != 0) {
		tile nt = tile::tile_type(tile::move_up_bit + t.props.moving - 1);
		int ntx = int(nt) % (m_tex.getSize().x / m_ts);
		int nty = int(nt) / (m_tex.getSize().x / m_ts);

		m_va_editor[i * 4].position.x  = x * m_ts;
		m_va_editor[i * 4].position.y  = y * m_ts;
		m_va_editor[i * 4].texCoords.x = ntx * m_ts;
		m_va_editor[i * 4].texCoords.y = nty * m_ts;

		m_va_editor[i * 4 + 1].position.x  = (x + 1) * m_ts;
		m_va_editor[i * 4 + 1].position.y  = y * m_ts;
		m_va_editor[i * 4 + 1].texCoords.x = (ntx + 1) * m_ts;
		m_va_editor[i * 4 + 1].texCoords.y = nty * m_ts;

		m_va_editor[i * 4 + 2].position.x  = (x + 1) * m_ts;
		m_va_editor[i * 4 + 2].position.y  = (y + 1) * m_ts;
		m_va_editor[i * 4 + 2].texCoords.x = (ntx + 1) * m_ts;
		m_va_editor[i * 4 + 2].texCoords.y = (nty + 1) * m_ts;

		m_va_editor[i * 4 + 3].position.x  = x * m_ts;
		m_va_editor[i * 4 + 3].position.y  = (y + 1) * m_ts;
		m_va_editor[i * 4 + 3].texCoords.x = ntx * m_ts;
		m_va_editor[i * 4 + 3].texCoords.y = (nty + 1) * m_ts;
	}
}

std::vector<std::pair<sf::Vector2f, tile>> tilemap::intersects(sf::FloatRect aabb) const {
	std::vector<std::pair<sf::Vector2f, tile>> ret;
	sf::IntRect rounded_aabb(aabb.left - 1, aabb.top - 1, 3, 3);

	// get all tiles around the player
	for (int x = rounded_aabb.left; x <= rounded_aabb.left + rounded_aabb.width; x++) {
		for (int y = rounded_aabb.top; y <= rounded_aabb.top + rounded_aabb.height; ++y) {
			tile t = get(x, y);
			sf::FloatRect tile_aabb(x, y, 1, 1);
			// add non-empty ones that intersect to the list
			if (t != tile::empty && tile_aabb.intersects(aabb)) {
				ret.push_back(std::make_pair(sf::Vector2f(x, y), t));
			}
		}
	}

	return ret;
}

void tilemap::set(int x, int y, tile t) {
	m_tiles[x + y * m_xs] = t;
	m_update_quad(x + y * m_xs);
}

tile tilemap::get(int x, int y) const {
	if (m_oob(x, y)) return tile::block;
	return m_tiles[x + y * m_xs];
}

tile tilemap::get(int i) const {
	if (m_oob(i)) return tile::block;
	return m_tiles[i];
}

const std::vector<tile>& tilemap::get() const {
	return m_tiles;
}

void tilemap::clear(int x, int y) {
	if (m_oob(x, y)) return;
	set(x, y, tile::empty);
}

sf::Vector2i tilemap::size() const {
	return { m_xs, m_ys };
}

int tilemap::tile_size() const {
	return m_ts;
}

int tilemap::count() const {
	return m_xs * m_ys;
}

void tilemap::set_editor_view(bool state) {
	m_editor = state;
}

void tilemap::m_update_quad(int i) {
	m_set_quad(i, m_tiles[i]);
}

bool tilemap::m_oob(int x, int y) const {
	return x < 0 || x >= m_xs || y < 0 || y >= m_ys;
}

bool tilemap::m_oob(int i) const {
	return i > m_tiles.size();
}

nlohmann::json tilemap::serialize() const {
	using nlohmann::json;
	json j = json::array();
	for (auto& t : m_tiles) {
		j.push_back(t.serialize());
	}
	return j;
}

void tilemap::deserialize(const nlohmann::json& j) {
	m_tiles.clear();
	for (auto& tile_data : j) {
		tile t;
		t.deserialize(tile_data);
		m_tiles.push_back(t);
	}
	m_flush_va();
}

sf::IntRect tilemap::calculate_texture_rect(tile t) const {
	sf::IntRect res;
	res.left = int(t) % (m_tex.getSize().x / m_ts);
	res.top	 = int(t) / (m_tex.getSize().x / m_ts);
	res.left *= m_ts;
	res.top *= m_ts;
	res.width  = m_ts;
	res.height = m_ts;
	return res;
}

// ! tile methods !

tile::tile(tile_type t, tile_props props)
	: type(t),
	  props(props) {
}

bool tile::operator==(const tile_type&& type) const {
	return type == this->type;
}

bool tile::operator!=(const tile_type&& type) const {
	return type != this->type;
}

// tile type defs

bool tile::harmful() const {
	return type == tile::spike;
}

bool tile::solid() const {
	return type == tile::block || type == tile::gravity;
}

// ///////////////////

tile::operator int() const {
	return int(type);
}

nlohmann::json tile::serialize() const {
	using nlohmann::json;
	json j = json::array();
	j[0]   = type;
	j[1]   = props.moving;
	return j;
}

void tile::deserialize(const nlohmann::json& j) {
	j.at(0).get_to(type);
	j.at(1).get_to(props.moving);
}
