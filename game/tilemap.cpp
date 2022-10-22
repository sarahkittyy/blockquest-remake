#include "tilemap.hpp"
#include <SFML/Graphics/RenderTarget.hpp>

#include "debug.hpp"
#include "util.hpp"

#include <iomanip>

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
	sf::VertexArray& va_to_modify = t.editor_only() ? m_va_editor : m_va;

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
	} else if (t != tile::stopper) {
		for (int k = 0; k < 4; ++k) {
			m_va_editor[i * 4 + k] = sf::Vertex();
		}
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
	t.m_x				  = x;
	t.m_y				  = y;
	m_tiles[x + y * m_xs] = t;
	m_update_quad(x + y * m_xs);
}

tile tilemap::get(int x, int y) const {
	if (m_oob(x, y)) return tile(tile::block, x, y);
	return m_tiles[x + y * m_xs];
}

tile tilemap::get(int i) const {
	if (m_oob(i)) return tile(tile::block, i / m_xs, i % m_ys);
	return m_tiles[i];
}

const std::vector<tile>& tilemap::get() const {
	return m_tiles;
}

void tilemap::clear(int x, int y) {
	if (m_oob(x, y)) return;
	set(x, y, tile::empty);
}

void tilemap::clear() {
	m_tiles.clear();
	m_tiles.resize(m_xs * m_ys, tile::empty);
	m_flush_va();
}

bool tilemap::in_bounds(sf::Vector2i pos) const {
	return pos.x >= 0 && pos.x < m_xs && pos.y >= 0 && pos.y < m_ys;
}

sf::Vector2i tilemap::size() const {
	return { m_xs, m_ys };
}

int tilemap::tile_size() const {
	return m_ts;
}

sf::Vector2f tilemap::total_size() const {
	return sf::Vector2f(size() * tile_size());
}

int tilemap::count() const {
	return m_xs * m_ys;
}

int tilemap::tile_count(tile::tile_type type) const {
	return std::count_if(m_tiles.cbegin(), m_tiles.cend(), [type](const tile& t) {
		return t.type == type;
	});
}

sf::Vector2i tilemap::find_first_of(tile::tile_type type) const {
	for (int x = 0; x < m_xs; ++x) {
		for (int y = 0; y < m_ys; ++y) {
			if (get(x, y) == type) {
				return { x, y };
			}
		}
	}
	return { -1, -1 };
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

std::string tilemap::save() const {
	std::ostringstream ss;
	ss << std::setfill('0');
	for (auto& tile : m_tiles) {
		if (tile == tile::empty) {
			ss << std::setw(1) << "/";
			continue;
		}
		ss << std::setw(2);
		ss << int(tile.type);
		ss << std::setw(1);
		ss << tile.props.moving;
	}
	return ss.str();
}

void tilemap::load(std::string str) {
	m_tiles.clear();
	m_tiles.resize(m_xs * m_ys, tile::empty);
	std::istringstream ss(str);
	for (int i = 0; i < m_xs * m_ys; ++i) {
		m_tiles[i].m_x = i % m_xs;
		m_tiles[i].m_y = i / m_xs;
		if (ss.peek() == '/') {
			m_tiles[i].type = tile::empty;
			char ch;
			ss >> ch;
			continue;
		}
		char buf[3];
		ss.get(buf, 3);
		m_tiles[i].type = static_cast<tile::tile_type>(std::atoi(buf));
		ss.get(buf, 2);
		m_tiles[i].props.moving = std::atoi(buf);
	}
	m_flush_va();
}

// ! tile methods !

tile::tile(tile_type t, tile_props props)
	: type(t),
	  props(props) {
}

tile::tile(tile_type t, int x, int y)
	: type(t),
	  props(tile_props()) {
	m_x = x;
	m_y = y;
}

bool tile::operator==(const tile_type&& type) const {
	return type == this->type;
}

bool tile::operator!=(const tile_type&& type) const {
	return type != this->type;
}

int tile::x() const {
	return m_x;
}

int tile::y() const {
	return m_y;
}

// tile type defs

bool tile::harmful() const {
	return type == tile::spike;
}

bool tile::solid() const {
	return type == tile::block ||
		   type == tile::gravity ||
		   type == tile::ice ||
		   type == tile::black ||
		   type == tile::ladder ||
		   type == tile::gravity;
}

bool tile::blocks_wallkicks() const {
	return type == tile::black;
}

bool tile::blocks_moving_tiles() const {
	return solid() || harmful() || type == tile::stopper;
}

bool tile::movable() const {
	return solid() || harmful();
}

bool tile::editor_only() const {
	return type == tile::stopper;
}

// ///////////////////

tile::operator int() const {
	return int(type);
}

std::string tile::description(tile_type type) {
	static const std::unordered_map<tile_type, std::string> map = {
		{ tile::empty, "An empty tile" },
		{ tile::begin, "The player's starting position" },
		{ tile::end, "The level's goal" },
		{ tile::block, "A normal, ordinary block" },
		{ tile::ice, "Like a normal block, but very slippery" },
		{ tile::black, "Like a normal block, but cannot be wallkicked off" },
		{ tile::gravity, "Flips gravity when stood atop" },
		{ tile::spike, "Danger! Avoid these" },
		{ tile::ladder, "A ladder that can be climbed" },
		// invisible tiles
		{ tile::stopper, "Invisible, moving tiles can interact with this but not the player." },
		{ tile::erase, "Erase tiles" },
		{ tile::move_up_bit, "For internal use only" },
		{ tile::move_right_bit, "For internal use only" },
		{ tile::move_down_bit, "For internal use only" },
		{ tile::move_left_bit, "For internal use only" },
		{ tile::move_up, "Makes a tile move upward" },
		{ tile::move_right, "Makes a tile move rightward" },
		{ tile::move_down, "Makes a tile move downward" },
		{ tile::move_left, "Makes a tile move leftward" },
		{ tile::move_none, "Makes a tile stationary again" },
		{ tile::cursor, "For internal use only" }
	};
	return map.at(type);
}
