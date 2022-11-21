#include "tilemap.hpp"
#include <SFML/Graphics/RenderTarget.hpp>

#include "debug.hpp"
#include "util.hpp"

#include <cmath>
#include <iomanip>
#include <unordered_set>

tilemap::tilemap(sf::Texture& tex, int xs, int ys, int ts, int tex_ts)
	: m_tex(tex), m_xs(xs), m_ys(ys), m_ts(ts), m_tex_ts(tex_ts), m_editor(false) {
	m_va.setPrimitiveType(sf::Quads);
	m_va_editor.setPrimitiveType(sf::Quads);
	m_va_arrows.setPrimitiveType(sf::Quads);
	m_tiles.resize(xs * ys, tile::empty);
	m_flush_va();
}

void tilemap::draw(sf::RenderTarget& t, sf::RenderStates s) const {
	s.transform *= getTransform();
	s.texture = &m_tex;
	t.draw(m_va, s);
	if (m_editor) {
		t.draw(m_va_editor, s);
		t.draw(m_va_arrows, s);
	}
}

void tilemap::m_flush_va() {
	m_va.clear();
	m_va.resize(m_xs * m_ys * 4);
	m_va_editor.clear();
	m_va_editor.resize(m_xs * m_ys * 4);
	m_va_arrows.clear();
	m_va_arrows.resize(m_xs * m_ys * 4);
	for (int i = 0; i < m_tiles.size(); ++i) {
		m_update_quad(i);
	}
}

void tilemap::m_set_quad(int i, tile t) {
	// x and y of current tile
	int x = i % m_xs;
	int y = i / m_xs;

	sf::Vertex air;

	if (t == tile::empty) {
		m_va[i * 4]			   = air;
		m_va[i * 4 + 1]		   = air;
		m_va[i * 4 + 2]		   = air;
		m_va[i * 4 + 3]		   = air;
		m_va_editor[i * 4]	   = air;
		m_va_editor[i * 4 + 1] = air;
		m_va_editor[i * 4 + 2] = air;
		m_va_editor[i * 4 + 3] = air;
		m_va_arrows[i * 4]	   = air;
		m_va_arrows[i * 4 + 1] = air;
		m_va_arrows[i * 4 + 2] = air;
		m_va_arrows[i * 4 + 3] = air;
		return;
	}

	int tx = int(t) % (m_tex.getSize().x / m_tex_ts);
	int ty = int(t) / (m_tex.getSize().x / m_tex_ts);

	// tiles only visible in editor mode
	sf::VertexArray& va_to_modify = t.editor_only() ? m_va_editor : m_va;
	// remove editor tiles on this spot if placing a non-editor tile on it
	if (!t.editor_only()) {
		m_va_editor[i * 4]	   = air;
		m_va_editor[i * 4 + 1] = air;
		m_va_editor[i * 4 + 2] = air;
		m_va_editor[i * 4 + 3] = air;
	} else {
		m_va[i * 4]		= air;
		m_va[i * 4 + 1] = air;
		m_va[i * 4 + 2] = air;
		m_va[i * 4 + 3] = air;
	}

	// fill in the quad
	va_to_modify[i * 4].position.x	= x * m_ts;
	va_to_modify[i * 4].position.y	= y * m_ts;
	va_to_modify[i * 4].texCoords.x = tx * m_tex_ts;
	va_to_modify[i * 4].texCoords.y = ty * m_tex_ts;

	va_to_modify[i * 4 + 1].position.x	= (x + 1) * m_ts;
	va_to_modify[i * 4 + 1].position.y	= y * m_ts;
	va_to_modify[i * 4 + 1].texCoords.x = (tx + 1) * m_tex_ts;
	va_to_modify[i * 4 + 1].texCoords.y = ty * m_tex_ts;

	va_to_modify[i * 4 + 2].position.x	= (x + 1) * m_ts;
	va_to_modify[i * 4 + 2].position.y	= (y + 1) * m_ts;
	va_to_modify[i * 4 + 2].texCoords.x = (tx + 1) * m_tex_ts;
	va_to_modify[i * 4 + 2].texCoords.y = (ty + 1) * m_tex_ts;

	va_to_modify[i * 4 + 3].position.x	= x * m_ts;
	va_to_modify[i * 4 + 3].position.y	= (y + 1) * m_ts;
	va_to_modify[i * 4 + 3].texCoords.x = tx * m_tex_ts;
	va_to_modify[i * 4 + 3].texCoords.y = (ty + 1) * m_tex_ts;

	// render movement arrows in editor mode
	if (t.props.moving != 0) {
		tile nt = tile::tile_type(tile::move_up_bit + t.props.moving - 1);
		int ntx = int(nt) % (m_tex.getSize().x / m_tex_ts);
		int nty = int(nt) / (m_tex.getSize().x / m_tex_ts);

		m_va_arrows[i * 4].position.x  = x * m_ts;
		m_va_arrows[i * 4].position.y  = y * m_ts;
		m_va_arrows[i * 4].texCoords.x = ntx * m_tex_ts;
		m_va_arrows[i * 4].texCoords.y = nty * m_tex_ts;

		m_va_arrows[i * 4 + 1].position.x  = (x + 1) * m_ts;
		m_va_arrows[i * 4 + 1].position.y  = y * m_ts;
		m_va_arrows[i * 4 + 1].texCoords.x = (ntx + 1) * m_tex_ts;
		m_va_arrows[i * 4 + 1].texCoords.y = nty * m_tex_ts;

		m_va_arrows[i * 4 + 2].position.x  = (x + 1) * m_ts;
		m_va_arrows[i * 4 + 2].position.y  = (y + 1) * m_ts;
		m_va_arrows[i * 4 + 2].texCoords.x = (ntx + 1) * m_tex_ts;
		m_va_arrows[i * 4 + 2].texCoords.y = (nty + 1) * m_tex_ts;

		m_va_arrows[i * 4 + 3].position.x  = x * m_ts;
		m_va_arrows[i * 4 + 3].position.y  = (y + 1) * m_ts;
		m_va_arrows[i * 4 + 3].texCoords.x = ntx * m_tex_ts;
		m_va_arrows[i * 4 + 3].texCoords.y = (nty + 1) * m_tex_ts;
	} else {
		for (int k = 0; k < 4; ++k) {
			m_va_arrows[i * 4 + k] = sf::Vertex();
		}
	}
}

std::vector<std::pair<sf::Vector2f, tile>> tilemap::intersects(sf::FloatRect aabb, bool roofs) const {
	std::vector<std::pair<sf::Vector2f, tile>> ret;
	sf::IntRect rounded_aabb(aabb.left - 1, aabb.top - 1, aabb.width + 3, aabb.height + 3);

	// get all tiles around the player
	for (int x = rounded_aabb.left; x <= rounded_aabb.left + rounded_aabb.width; x++) {
		for (int y = rounded_aabb.top; y <= rounded_aabb.top + rounded_aabb.height; ++y) {
			tile t;
			if (m_oob(x, y) && roofs) {
				t = tile(tile::block, x, y);
			} else {
				t = get(x, y);
			}
			sf::FloatRect tile_aabb(x, y, 1, 1);
			// add non-empty ones that intersect to the list
			if (t != tile::empty && tile_aabb.intersects(aabb)) {
				ret.push_back(std::make_pair(sf::Vector2f(x, y), t));
			}
		}
	}

	return ret;
}

std::optional<tilemap::diff> tilemap::set(int x, int y, tile t) {
	if (m_oob(x, y)) return {};

	tilemap::diff d;
	d.x		 = x;
	d.y		 = y;
	d.before = m_tiles[x + y * m_xs];
	d.after	 = t;

	t.m_x				  = x;
	t.m_y				  = y;
	m_tiles[x + y * m_xs] = t;
	m_update_quad(x + y * m_xs);
	return d;
}

std::vector<tilemap::diff> tilemap::set_line(sf::Vector2i min, sf::Vector2i max, tile tl) {
	std::vector<tilemap::diff> ret;

	const sf::Vector2i dist = max - min;
	std::unordered_set<sf::Vector2i, util::vector_hash<int>> already_passed;
	// will do 45 iterations (max necessary for diagonal across map)
	for (float t = 0; t <= 1; t += 0.022f) {
		sf::Vector2i lerped(
			std::round(util::lerp(min.x, max.x, t)),
			std::round(util::lerp(min.y, max.y, t)));
		if (already_passed.contains(lerped)) continue;
		already_passed.insert(lerped);
		auto diff = set(lerped.x, lerped.y, tl);
		if (diff) {
			ret.push_back(*diff);
		}
	}
	return ret;
}

tile tilemap::get(int x, int y) const {
	if (m_oob(x, y)) return m_oob_tile(x, y);
	return m_tiles[x + y * m_xs];
}

tile tilemap::get(int i) const {
	if (m_oob(i)) return m_oob_tile(i / m_xs, i % m_ys);
	return m_tiles[i];
}

const std::vector<tile>& tilemap::get() const {
	return m_tiles;
}

std::optional<tilemap::diff> tilemap::clear(int x, int y) {
	return set(x, y, tile::empty);
}

std::vector<tilemap::diff> tilemap::clear() {
	std::vector<tilemap::diff> ret;
	for (int x = 0; x < m_xs; ++x) {
		for (int y = 0; y < m_ys; ++y) {
			ret.push_back({
				.x		= x,
				.y		= y,
				.before = m_tiles[x + y * m_xs].type,
				.after	= tile::empty,
			});
		}
	}
	m_tiles.clear();
	m_tiles.resize(m_xs * m_ys, tile::empty);
	m_flush_va();
	return ret;
}

void tilemap::undo(diff d) {
	set(d.x, d.y, d.before);
}

void tilemap::redo(diff d) {
	set(d.x, d.y, d.after);
}

std::vector<tilemap::diff> tilemap::layer_over(tilemap& target, bool override) const {
	std::vector<tilemap::diff> ret;
	for (int x = 0; x < m_xs; ++x) {
		for (int y = 0; y < m_ys; ++y) {
			tile here = get(x, y);
			if (here == tile::empty) continue;
			tile there = target.get(x, y);
			if (override || there != tile::empty) {
				if (here == tile::erase) {
					auto diff = target.clear(x, y);
					if (diff)
						ret.push_back(*diff);
				} else {
					auto diff = target.set(x, y, here);
					if (diff)
						ret.push_back(*diff);
				}
			}
		}
	}
	return ret;
}

void tilemap::copy_to(tilemap& target) const {
	for (int x = 0; x < m_xs; ++x) {
		for (int y = 0; y < m_ys; ++y) {
			target.set(x, y, get(x, y));
		}
	}
}

tile tilemap::m_oob_tile(int x, int y) const {
	if (x < 0 || x >= m_xs) {
		return tile(tile::block, x, y);
	} else if (y >= m_ys || y < 0) {
		return tile(tile::empty, x, y);
	} else {
		return tile(tile::block, x, y);
	}
}

bool tilemap::in_bounds(sf::Vector2i pos) const {
	return pos.x >= 0 && pos.x < m_xs && pos.y >= 0 && pos.y < m_ys;
}
ImRect tilemap::calc_uvs(tile::tile_type type, sf::Texture& tex, int tile_size) {
	// size of the tiles texture in tiles
	sf::Vector2i tex_sz(
		tex.getSize().x / tile_size,
		tex.getSize().y / tile_size);
	// size of a tile in uv coords (0-1)
	sf::Vector2f tsz(
		1.f / tex_sz.x,
		1.f / tex_sz.y);
	int x = type % tex_sz.x;
	int y = type / tex_sz.x;
	sf::Vector2f uv0(x * tsz.x, y * tsz.y);
	return ImRect(uv0, uv0 + tsz);
}

ImRect tilemap::calc_uvs(tile::tile_type type) const {
	return tilemap::calc_uvs(type, m_tex, m_tex_ts);
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
	res.left = int(t) % (m_tex.getSize().x / m_tex_ts);
	res.top	 = int(t) / (m_tex.getSize().x / m_tex_ts);
	res.left *= m_tex_ts;
	res.top *= m_tex_ts;
	res.width  = m_tex_ts;
	res.height = m_tex_ts;
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
		m_tiles[i].m_x = int(i % m_xs);
		m_tiles[i].m_y = int(i / m_xs);
		if (ss.peek() == '/') {
			m_tiles[i].type = tile::empty;
			char ch;
			ss >> ch;
			continue;
		}
		try {
			char buf[3];
			ss.get(buf, 3);
			m_tiles[i].type = static_cast<tile::tile_type>(std::stoi(buf));
			ss.get(buf, 2);
			m_tiles[i].props.moving = std::stoi(buf);
		} catch (std::invalid_argument const& e) {
			m_tiles[i] = tile::empty;
		}
	}
	m_flush_va();
}

bool tilemap::diffs_equal(std::vector<tilemap::diff> a, std::vector<tilemap::diff> b) {
	if (a.size() != b.size()) return false;
	for (int i = 0; i < a.size(); ++i) {
		tilemap::diff a0 = a[i];
		bool found_match = false;
		for (int j = 0; j < b.size(); ++j) {
			tilemap::diff b0 = a[j];
			if (a0.x == b0.x && a0.y == b0.y && a0.after == b0.after) {
				found_match = true;
				break;
			}
		}
		if (!found_match) {
			return false;
		}
	}
	return true;
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

bool tile::eq(const tile& other) const {
	return type == other.type && props.moving == other.props.moving;
}

float tile::x() const {
	return m_x;
}

float tile::y() const {
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
	return solid() || harmful() || type == tile::stopper;
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
		{ tile::move_up, "Tile will move up" },
		{ tile::move_right, "Tile will move right" },
		{ tile::move_down, "Tile will move down" },
		{ tile::move_left, "Tile will move left" },
		{ tile::move_none, "Tile is stationary" },
		{ tile::cursor, "For internal use only" }
	};
	return map.at(type);
}
