#include "moving_tile.hpp"

#include "debug.hpp"

#include <algorithm>

////////////////////// MANAGER METHODS //////////////////////////////

moving_tile_manager::moving_tile_manager(resource& r, tilemap& t)
	: m_tmap(t),
	  m_r(r) {
	// initialize all moving tiles
	for (int y = 0; y < t.size().y; ++y) {
		for (int x = 0; x < t.size().x; ++x) {
			tile tl = t.get(x, y);
			if (tl.props.moving != 0) {
				m_tiles.push_back(moving_tile(x, y, t, r));
			}
		}
	}
}

void moving_tile_manager::draw(sf::RenderTarget& t, sf::RenderStates s) const {
	s.transform *= getTransform();
	for (auto& tile : m_tiles) {
		t.draw(tile, s);
	}
}

std::vector<std::pair<sf::Vector2f, tile>> moving_tile_manager::intersects(sf::FloatRect aabb) const {
	std::vector<std::pair<sf::Vector2f, tile>> contacts;

	for (auto& tile : m_tiles) {
		if (tile.get_aabb().intersects(aabb)) {
			contacts.push_back(std::make_pair(sf::Vector2f(tile.m_xp, tile.m_yp), tile));
		}
	}

	return contacts;
}

std::vector<std::pair<sf::Vector2f, moving_tile>> moving_tile_manager::intersects_raw(sf::FloatRect aabb) const {
	std::vector<std::pair<sf::Vector2f, moving_tile>> contacts;

	for (auto& tile : m_tiles) {
		if (tile.get_aabb().intersects(aabb)) {
			contacts.push_back(std::make_pair(sf::Vector2f(tile.m_xp, tile.m_yp), tile));
		}
	}

	return contacts;
}

void moving_tile_manager::update(sf::Time dt) {
	// for every tile...
	sf::Vector2f sz = moving_tile::size();
	for (int i = 0; i < m_tiles.size(); ++i) {
		moving_tile& t = m_tiles[i];
		// intended next position
		float new_xp = t.m_xp + t.m_xv * dt.asSeconds();
		float new_yp = t.m_yp + t.m_yv * dt.asSeconds();
		float new_xv = t.m_xv;
		float new_yv = t.m_yv;

		// check for static collision
		sf::FloatRect aabb = t.get_ghost_aabb(new_xp, new_yp);
		debug::get().box(util::scale(aabb, float(m_tmap.tile_size())), sf::Color::Red);
		auto contacts = m_tmap.intersects(aabb);
		decltype(contacts) solid_contacts;
		std::copy_if(contacts.begin(), contacts.end(), std::back_inserter(solid_contacts), [](std::pair<sf::Vector2f, tile> t) {
			return t.second.blocks_moving_tiles();
		});
		if (m_handle_contact(solid_contacts)) {
			auto [pos, tile] = *solid_contacts.begin();
			if (std::abs(new_xv) > 0.01f) {
				new_xp = new_xp > pos.x
							 ? pos.x + 1	// hitting right side of block
							 : pos.x - 1;	// hitting left side of block
				new_xv = -new_xv;
			} else if (std::abs(new_yv) > 0.01f) {
				new_yp = new_yp > pos.y
							 ? pos.y + 1	// hitting bottom of block
							 : pos.y - 1;	// hitting top of block
				new_yv = -new_yv;
			}
		} else {
			// check for collision between other moving tiles
			for (int j = 0; j < m_tiles.size(); ++j) {
				if (i == j) continue;
				moving_tile& t2 = m_tiles[j];
				if (
					(util::same_sign(t2.vel().x, t.vel().x) && util::neither_zero(t.vel().x, t2.vel().x)) ||   //
					(util::same_sign(t2.vel().y, t.vel().y) && util::neither_zero(t.vel().y, t2.vel().y))	   //
				) { continue; }
				sf::FloatRect tile_ghost_aabb = std::abs(new_xv) > 0.01f ? t2.get_ghost_aabb_x() : t2.get_ghost_aabb_y();
				sf::FloatRect tile_aabb		  = t2.get_aabb();
				sf::Vector2f sz				  = moving_tile::size();
				if (aabb.intersects(tile_ghost_aabb)) {
					if (std::abs(new_xv) > 0.01f) {
						new_xp = new_xp > tile_aabb.left
									 ? tile_aabb.left + 1	 // hitting right side of block
									 : tile_aabb.left - 1;	 // hitting left side of block
						new_xv = -new_xv;
						if (t2.m_yv == 0) {
							t2.m_xv = -t2.m_xv;
						}
					} else if (std::abs(new_yv) > 0.01f) {
						new_yp = new_yp > tile_aabb.top
									 ? tile_aabb.top + 1	// hitting bottom of block
									 : tile_aabb.top - 1;	// hitting top of block
						new_yv = -new_yv;
						if (t2.m_xv == 0) {
							t2.m_yv = -t2.m_yv;
						}
					}
					break;
				}
			}
		}

		t.m_xp = new_xp;
		t.m_yp = new_yp;
		t.m_xv = new_xv;
		t.m_yv = new_yv;
		t.m_sync_position();
	}
}

bool moving_tile_manager::m_handle_contact(std::vector<std::pair<sf::Vector2f, tile>> contacts) {
	return contacts.size() != 0;
}

void moving_tile_manager::restart() {
	for (auto& tile : m_tiles) {
		tile.m_restart();
	}
}

////////////////////// MOVING TILE METHODS //////////////////////////////

moving_tile::moving_tile(int x, int y, tilemap& m, resource& r)
	: m_t(),
	  m_tmap(m),
	  m_r(r) {
	m_t = m_tmap.get(x, y);
	m_tmap.clear(x, y);
	m_start_dir = dir(m_t.props.moving - 1);
	m_start_x	= x;
	m_start_y	= y;
	// set the sprite texture & texture rect
	m_spr.setTexture(m_r.tex("assets/tiles.png"));
	m_spr.setTextureRect(m_tmap.calculate_texture_rect(m_t));

	m_restart();
}

void moving_tile::draw(sf::RenderTarget& t, sf::RenderStates s) const {
	s.transform *= getTransform();
	t.draw(m_spr, s);
}

sf::Vector2f moving_tile::size() {
	return { 1.f, 1.f };
}

sf::FloatRect moving_tile::get_aabb() const {
	return get_aabb(m_xp, m_yp);
}

sf::FloatRect moving_tile::get_ghost_aabb() const {
	return get_ghost_aabb(m_xp, m_yp);
}

moving_tile::operator tile() const {
	return m_t;
}

sf::Vector2f moving_tile::vel() const {
	return { m_xv, m_yv };
}

sf::Vector2f moving_tile::pos() const {
	return { m_xp, m_yp };
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
	if (m_xv == 0) {
		aabb.left += 0.05f;
		aabb.width -= 0.1f;
	} else if (m_yv == 0) {
		aabb.top += 0.05f;
		aabb.height -= 0.1f;
	}
	return aabb;
}

sf::FloatRect moving_tile::get_ghost_aabb_y() const {
	// ghost AABB will be normal sized on the axis parallel to motion, and
	// shrunk on the perpendicular axis
	// as perpendicular moving platforms should never interact?
	sf::FloatRect aabb = get_aabb();
	aabb.left += 0.05f;
	aabb.width -= 0.1f;
	return aabb;
}

sf::FloatRect moving_tile::get_ghost_aabb_x() const {
	// ghost AABB will be normal sized on the axis parallel to motion, and
	// shrunk on the perpendicular axis
	// as perpendicular moving platforms should never interact?
	sf::FloatRect aabb = get_aabb();
	aabb.top += 0.05f;
	aabb.height -= 0.1f;
	return aabb;
}

void moving_tile::m_sync_position() {
	m_spr.setPosition(m_xp * m_spr.getTextureRect().width, m_yp * m_spr.getTextureRect().height);
}

void moving_tile::m_restart() {
	m_xp = m_start_x;
	m_yp = m_start_y;

	m_xv = 0;
	m_yv = 0;

	switch (m_start_dir) {
	case up:
		m_yv = -phys.vel;
		break;
	case down:
		m_yv = phys.vel;
		break;
	case left:
		m_xv = -phys.vel;
		break;
	case right:
		m_xv = phys.vel;
		break;
	}

	m_sync_position();
}
