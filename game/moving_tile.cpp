#include "moving_tile.hpp"

#include "debug.hpp"
#include "resource.hpp"
#include "tilemap.hpp"

#include <algorithm>

static sf::Vector2f tile_dir_vel(moving_blob::dir d, float v) {
	sf::Vector2f res(0, 0);
	switch (d) {
	case moving_blob::up:
		res.y = -v;
		break;
	case moving_blob::down:
		res.y = v;
		break;
	case moving_blob::left:
		res.x = -v;
		break;
	case moving_blob::right:
		res.x = v;
		break;
	}
	return res;
}

////////////////////// MANAGER METHODS //////////////////////////////

moving_tile_manager::moving_tile_manager(tilemap& t)
	: m_tmap(t) {
	// initialize all moving tiles
	pos_set checked;
	for (int y = 0; y < t.size().y; ++y) {
		for (int x = 0; x < t.size().x; ++x) {
			if (checked.contains({ x, y })) {
				continue;
			}
			tile tl = t.get(x, y);
			if (tl.props.moving != 0) {
				moving_blob b(t);
				b.init(x, y, checked);
				m_blobs.push_back(b);
			} else {
				checked.insert({ x, y });
			}
		}
	}
}

void moving_tile_manager::draw(sf::RenderTarget& t, sf::RenderStates s) const {
	s.transform *= getTransform();
	for (auto& blob : m_blobs) {
		t.draw(blob, s);
	}
}

std::vector<std::pair<sf::Vector2f, tile>> moving_tile_manager::intersects(sf::FloatRect aabb) const {
	std::vector<std::pair<sf::Vector2f, tile>> contacts;

	for (auto& blob : m_blobs) {
		blob.intersects(aabb, contacts);
	}

	return contacts;
}

std::vector<std::pair<sf::Vector2f, moving_tile>> moving_tile_manager::intersects_raw(sf::FloatRect aabb) const {
	std::vector<std::pair<sf::Vector2f, moving_tile>> contacts;

	for (auto& blob : m_blobs) {
		blob.intersects_raw(aabb, contacts);
	}

	return contacts;
}

void moving_tile_manager::update(sf::Time dt) {
	// for every tile...
	for (int i = 0; i < m_blobs.size(); ++i) {
		moving_blob& b = m_blobs[i];
		// intended next position
		float new_xp = b.m_xp + b.m_xv * dt.asSeconds();
		float new_yp = b.m_yp + b.m_yv * dt.asSeconds();
		float new_xv = b.m_xv;
		float new_yv = b.m_yv;

		// check for static collision
		sf::FloatRect aabb = b.get_ghost_aabb(new_xp, new_yp);
		debug::get().box(util::scale(aabb, float(m_tmap.tile_size())), sf::Color::Red);
		auto contacts = m_tmap.intersects(aabb, true);
		decltype(contacts) solid_contacts;
		std::copy_if(contacts.begin(), contacts.end(), std::back_inserter(solid_contacts), [](std::pair<sf::Vector2f, tile> t) {
			return t.second.blocks_moving_tiles();
		});
		if (m_handle_contact(solid_contacts)) {
			auto [pos, tile] = *solid_contacts.begin();
			if (std::abs(new_xv) > 0.01f) {
				new_xp = new_xp > pos.x
							 ? pos.x + 1			 // hitting right side of block
							 : pos.x - aabb.width;	 // hitting left side of block
				new_xv = -new_xv;
			} else if (std::abs(new_yv) > 0.01f) {
				new_yp = new_yp > pos.y
							 ? pos.y + 1			  // hitting bottom of block
							 : pos.y - aabb.height;	  // hitting top of block
				new_yv = -new_yv;
			}
		} else {
			// check for collision between other moving tiles
			for (int j = 0; j < m_blobs.size(); ++j) {
				if (i == j) continue;
				moving_blob& t2 = m_blobs[j];
				if (
					(util::same_sign(t2.vel().x, b.vel().x) && util::neither_zero(b.vel().x, t2.vel().x)) ||   //
					(util::same_sign(t2.vel().y, b.vel().y) && util::neither_zero(b.vel().y, t2.vel().y))	   //
				) { continue; }
				sf::FloatRect tile_ghost_aabb = t2.get_ghost_aabb();
				sf::FloatRect tile_aabb		  = t2.get_aabb();
				if (aabb.intersects(tile_ghost_aabb)) {
					if (std::abs(new_xv) > 0.01f) {
						new_xp = new_xp > tile_aabb.left
									 ? tile_aabb.left + tile_aabb.width	  // hitting right side of block
									 : tile_aabb.left - aabb.width;		  // hitting left side of block
						new_xv = -new_xv;
						if (t2.m_yv == 0) {
							t2.m_xv = -t2.m_xv;
						}
					} else if (std::abs(new_yv) > 0.01f) {
						new_yp = new_yp > tile_aabb.top
									 ? tile_aabb.top + tile_aabb.height	  // hitting bottom of block
									 : tile_aabb.top - aabb.height;		  // hitting top of block
						new_yv = -new_yv;
						if (t2.m_xv == 0) {
							t2.m_yv = -t2.m_yv;
						}
					}

					break;
				}
			}
		}

		b.m_xp = new_xp;
		b.m_yp = new_yp;
		b.m_xv = new_xv;
		b.m_yv = new_yv;
		b.m_sync_position();
	}
}

bool moving_tile_manager::m_handle_contact(std::vector<std::pair<sf::Vector2f, tile>> contacts) {
	return contacts.size() != 0;
}

void moving_tile_manager::restart() {
	for (auto& tile : m_blobs) {
		tile.m_restart();
	}
}

////////////////////// BLOB METHODS /////////////////////////////////

moving_blob::moving_blob(tilemap& m)
	: m_tmap(m),
	  m_initialized(false),
	  m_min_xp(999),
	  m_min_yp(999),
	  m_max_xp(-1),
	  m_max_yp(-1),
	  m_xp(999),
	  m_yp(999),
	  m_xv(0),
	  m_yv(0) {
}

void moving_blob::init(int x, int y, pos_set& checked) {
	if (checked.contains({ x, y })) {
		return;
	}
	checked.insert({ x, y });
	if (m_initialized) {
		// match only tiles moving in the exact same direction & also movable
		tile t = m_tmap.get(x, y);
		if (dir(t.props.moving - 1) != m_start_dir || !t.movable()) {
			return;
		}
	}
	moving_tile mt(m_tmap.get(x, y), m_tmap);
	m_tiles.push_back(mt);
	m_tmap.clear(x, y);

	bool is_this_recurse_main = false;
	if (!m_initialized) {
		m_initialized		 = true;
		m_start_dir			 = dir(tile(mt).props.moving - 1);
		sf::Vector2f iv		 = tile_dir_vel(m_start_dir, phys.vel);
		m_xv				 = iv.x;
		m_yv				 = iv.y;
		is_this_recurse_main = true;
	}
	m_min_xp = std::min<int>(m_min_xp, x);
	m_min_yp = std::min<int>(m_min_yp, y);
	m_max_xp = std::max<int>(m_max_xp, x);
	m_max_yp = std::max<int>(m_max_yp, y);

	if (std::abs(m_xv) > 0.01f) {
		// check on left and right
		init(x - 1, y, checked);
		init(x + 1, y, checked);
	} else if (std::abs(m_yv) > 0.01f) {
		init(x, y - 1, checked);
		init(x, y + 1, checked);
	}

	// final cleanup steps
	if (is_this_recurse_main) {
		m_xp = m_min_xp;
		m_yp = m_min_yp;

		m_start_x = m_xp;
		m_start_y = m_yp;

		// sort the tiles from min to max
		std::sort(m_tiles.begin(), m_tiles.end(), [](const moving_tile& a, const moving_tile& b) -> bool {
			return a.m_start_x < b.m_start_x || a.m_start_y < b.m_start_y;
		});
		// set the tiles positions
		for (int i = 0; i < m_tiles.size(); ++i) {
			moving_tile& s = m_tiles[i];
			if (std::abs(m_xv) > 0.01f) {
				s.setPosition(m_tmap.tile_size() * i, 0);
			} else if (std::abs(m_yv) > 0.01f) {
				s.setPosition(0, m_tmap.tile_size() * i);
			}
		}

		// just in case :3
		m_sync_position();
		m_sync_position();
	}
}

void moving_blob::intersects(sf::FloatRect aabb, std::vector<std::pair<sf::Vector2f, tile>>& out) const {
	for (int i = 0; i < m_tiles.size(); ++i) {
		const moving_tile& tile = m_tiles[i];
		if (tile.get_aabb().intersects(aabb)) {
			out.push_back(std::make_pair(tile.pos(), tile));
		}
	}
}

void moving_blob::intersects_raw(sf::FloatRect aabb, std::vector<std::pair<sf::Vector2f, moving_tile>>& out) const {
	for (int i = 0; i < m_tiles.size(); ++i) {
		const moving_tile& tile = m_tiles[i];
		if (tile.get_aabb().intersects(aabb)) {
			out.push_back(std::make_pair(tile.pos(), tile));
		}
	}
}

sf::FloatRect moving_blob::get_aabb() const {
	return get_aabb(m_xp, m_yp);
}

sf::FloatRect moving_blob::get_ghost_aabb() const {
	return get_ghost_aabb(m_xp, m_yp);
}

sf::FloatRect moving_blob::get_aabb(float x, float y) const {
	sf::Vector2f sz = size();
	return sf::FloatRect(x, y, sz.x, sz.y);
}

sf::FloatRect moving_blob::get_ghost_aabb(float x, float y) const {
	// ghost AABB will be normal sized on the axis parallel to motion, and
	// shrunk on the perpendicular axis
	// as perpendicular moving platforms should never interact?
	sf::FloatRect aabb = get_aabb(x, y);
	if (std::abs(m_xv) < 0.01f) {
		aabb.left += 0.05f;
		aabb.width -= 0.1f;
	} else if (std::abs(m_yv) < 0.01f) {
		aabb.top += 0.05f;
		aabb.height -= 0.1f;
	}
	return aabb;
}

sf::Vector2f moving_blob::vel() const {
	return { m_xv, m_yv };
}

sf::Vector2f moving_blob::pos() const {
	return { m_xp, m_yp };
}

sf::Vector2f moving_blob::size() const {
	return sf::Vector2f(m_max_xp - m_min_xp + 1, m_max_yp - m_min_yp + 1);
}

void moving_blob::draw(sf::RenderTarget& t, sf::RenderStates s) const {
	s.transform *= getTransform();
	for (auto& tile : m_tiles) {
		t.draw(tile, s);
	}
}

sf::Vector2f moving_blob::m_tpos(int idx) const {
	sf::Vector2f base_pos(m_xp, m_yp);
	if (m_start_dir == dir::up || m_start_dir == dir::down) {
		base_pos.y += idx;
	} else if (m_start_dir == dir::left || m_start_dir == dir::right) {
		base_pos.x += idx;
	}
	return base_pos;
}

void moving_blob::m_sync_position() {
	setPosition(m_xp * m_tmap.tile_size(), m_yp * m_tmap.tile_size());
	m_sync_tiles();
}

void moving_blob::m_restart() {
	m_xp = m_start_x;
	m_yp = m_start_y;

	sf::Vector2f iv = tile_dir_vel(m_start_dir, phys.vel);
	m_xv			= iv.x;
	m_yv			= iv.y;

	m_sync_position();
}

void moving_blob::m_sync_tiles() {
	for (int i = 0; i < m_tiles.size(); ++i) {
		moving_tile& tile = m_tiles[i];
		sf::Vector2f tpos = m_tpos(i);
		tile.m_last_xp	  = tile.m_xp;
		tile.m_last_yp	  = tile.m_yp;
		tile.m_xp		  = tpos.x;
		tile.m_yp		  = tpos.y;
		tile.m_xv		  = m_xv;
		tile.m_yv		  = m_yv;
		tile.m_t.m_x	  = tile.m_xp;
		tile.m_t.m_y	  = tile.m_yp;
	}
}

////////////////////// MOVING TILE METHODS //////////////////////////////

moving_tile::moving_tile(tile t, tilemap& m)
	: m_t(t) {
	m_start_x = t.x();
	m_start_y = t.y();
	// set the sprite texture & texture rect
	m_spr.setTexture(resource::get().tex("assets/tiles.png"));
	m_spr.setTextureRect(m.calculate_texture_rect(m_t));
}

void moving_tile::draw(sf::RenderTarget& t, sf::RenderStates s) const {
	if (m_t.editor_only()) return;
	s.transform *= getTransform();
	t.draw(m_spr, s);
}

sf::Vector2f moving_tile::size() {
	return { 1.f, 1.f };
}

sf::Vector2f moving_tile::pos() const {
	return { m_xp, m_yp };
}

sf::Vector2f moving_tile::vel() const {
	return { m_xv, m_yv };
}

sf::Vector2f moving_tile::delta() const {
	return { m_xp - m_last_xp, m_yp - m_last_yp };
}

moving_tile::operator tile() const {
	return m_t;
}

sf::FloatRect moving_tile::get_aabb() const {
	return get_aabb(m_xp, m_yp);
}

sf::FloatRect moving_tile::get_ghost_aabb() const {
	return get_ghost_aabb(m_xp, m_yp);
}

sf::FloatRect moving_tile::get_aabb(float x, float y) const {
	sf::Vector2f sz = size();
	return sf::FloatRect(x + (1 - sz.x) / 2.f, y + (1 - sz.y) / 2.f, sz.x, sz.y);
}

sf::FloatRect moving_tile::get_ghost_aabb(float x, float y) const {
	// ghost AABB will be normal sized on the axis parallel to motion, and
	// shrunk on the perpendicular axis
	// as perpendicular moving platforms should never interact?
	sf::FloatRect aabb = get_aabb(x, y);
	if (std::abs(m_xv) < 0.01f) {
		aabb.left += 0.05f;
		aabb.width -= 0.1f;
	} else {
		aabb.top += 0.05f;
		aabb.height -= 0.1f;
	}
	return aabb;
}

