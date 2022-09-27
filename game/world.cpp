#include "world.hpp"

#include "debug.hpp"
#include "imgui_internal.h"
#include "util.hpp"

world::world(resource& r, level l)
	: m_r(r),
	  m_tmap(l.map()),
	  m_mt_mgr(m_r, m_tmap),
	  m_level(l),
	  m_player(r.tex("assets/player.png")),
	  m_start_x(0),
	  m_start_y(0),
	  m_moving_platform_handle() {
	m_player.set_animation("walk");
	m_player.setOrigin(m_player.size().x / 2.f, m_player.size().y / 2.f);
	m_init_world();
}

void world::m_init_world() {
	// find the start position
	auto& tiles		= m_tmap.get();
	auto start_tile = std::find(tiles.cbegin(), tiles.cend(), tile::begin);
	if (start_tile != tiles.cend()) {
		int index = std::distance(tiles.cbegin(), start_tile);
		m_start_x = index % m_tmap.size().x;
		m_start_y = index / m_tmap.size().x;
	}

	// set the world up at the start
	m_restart_world();
}

void world::m_restart_world() {
	// move the player to the start
	m_xp		   = m_start_x + 0.499f;
	m_yp		   = m_start_y + 0.4f;
	m_xv		   = 0;
	m_yv		   = 0;
	m_flip_gravity = false;
	m_dead		   = false;
	m_sync_player_position();
	m_player.setScale(1, 1);
	m_mt_mgr.restart();
}

/*
http://higherorderfun.com/blog/2012/05/20/the-guide-to-implementing-2d-platformers/
- Decompose movement into X and Y axes, step one at a time. If you’re planning on implementing slopes afterwards, step X first, then Y. Otherwise, the order shouldn’t matter much. Then, for each axis:
- Get the coordinate of the forward-facing edge, e.g. : If walking left, the x coordinate of left of bounding box. If walking right, x coordinate of right side. If up, y coordinate of top, etc.
- Figure which lines of tiles the bounding box intersects with – this will give you a minimum and maximum tile value on the OPPOSITE axis. For example, if we’re walking left, perhaps the player intersects with horizontal rows 32, 33 and 34 (that is, tiles with y = 32 * TS, y = 33 * TS, and y = 34 * TS, where TS = tile size).
- Scan along those lines of tiles and towards the direction of movement until you find the closest static obstacle. Then loop through every moving obstacle, and determine which is the closest obstacle that is actually on your path.
- The total movement of the player along that direction is then the minimum between the distance to closest obstacle, and the amount that you wanted to move in the first place.
- Move player to the new position. With this new position, step the other coordinate, if still not done.
*/

void world::update(sf::Time dt) {
	auto keyed = sf::Keyboard::isKeyPressed;
	using Key  = sf::Keyboard;

	// deaths are handled here to deal with dying mid-logic
	if (m_dead) {
		m_dead = false;
		m_restart_world();
	}

	// update the player's animations
	m_player.update();

	// -1 if gravity is flipped. used for y velocity calculations
	float gravity_sign = m_flip_gravity ? -1 : 1;

	// update moving platforms
	m_mt_mgr.update(dt);

	// shift the player by the amount the platform they're on moved
	sf::Vector2f mp_offset = m_mp_player_offset(dt);
	m_xp += mp_offset.x;
	m_yp += mp_offset.y;

	// controls
	if (keyed(Key::Right)) {
		// wallkicking
		// normal acceleration
		if (m_xv < 0) {
			m_xv += phys.x_decel * dt.asSeconds();
		}
		m_xv += phys.x_accel * dt.asSeconds();
		m_player.setScale(-1, gravity_sign);
	} else if (keyed(Key::Left)) {
		// wallkicking
		// normal acceleration
		if (m_xv > 0) {
			m_xv -= phys.x_decel * dt.asSeconds();
		}
		m_xv -= phys.x_accel * dt.asSeconds();
		m_player.setScale(1, gravity_sign);
	} else {
		if (m_xv > (phys.x_decel / 2.f) * dt.asSeconds()) {
			m_xv -= phys.x_decel * dt.asSeconds();
		} else if (m_xv < (-phys.x_decel / 2.f) * dt.asSeconds()) {
			m_xv += phys.x_decel * dt.asSeconds();
		} else {
			m_xv = 0;
		}
	}

	if (keyed(Key::Up) && m_player_grounded() && !m_tile_above_player()) {
		m_yv = -phys.jump_v * gravity_sign;
		m_r.play_sound("jump");
	}
	/////////////////////

	m_xv = util::clamp(m_xv, -phys.xv_max, phys.xv_max);

	// gravity
	m_yv += phys.grav * dt.asSeconds() * gravity_sign;
	if (m_flip_gravity) {
		m_yv = util::clamp(m_yv, -phys.yv_max, phys.jump_v);
	} else {
		m_yv = util::clamp(m_yv, -phys.jump_v, phys.yv_max);
	}

	// !! physics !! //

	float initial_x	 = m_xp;
	float initial_y	 = m_yp;
	float intended_x = m_xp + m_xv * dt.asSeconds();
	float intended_y = m_yp + m_yv * dt.asSeconds();

	bool x_collided = false;
	bool y_collided = false;

	float cx = initial_x, cy = initial_y;

	debug::get().box(util::scale<float>(m_get_player_top_ghost_aabb(cx, cy), m_player.size().x));
	debug::get().box(util::scale<float>(m_get_player_bottom_ghost_aabb(cx, cy), m_player.size().x));
	debug::get().box(util::scale<float>(m_get_player_left_ghost_aabb(cx, cy), m_player.size().x));
	debug::get().box(util::scale<float>(m_get_player_right_ghost_aabb(cx, cy), m_player.size().x));

	// subdivide the movement into x and y steps
	for (float t = 0; t < 1.0f && !m_dead; t += 0.1f) {
		if (!x_collided) {
			// x
			cx = util::lerp(initial_x, intended_x, t);

			// check x collision
			sf::FloatRect aabb	  = m_get_player_x_aabb(cx, cy);
			auto collided_static  = m_tmap.intersects(aabb);
			auto collided_dynamic = m_mt_mgr.intersects(aabb);
			auto collided		  = util::merge_contacts(collided_static, collided_dynamic);

			if (m_handle_contact(cx, cy, collided)) {
				// retrieve the first collision
				// might want to check all collisions in the future
				auto [pos, tile] = m_first_solid(collided);
				intended_x		 = cx > pos.x
									   ? pos.x + 1.5f - ((1 - m_player_size().x) / 2.f)	   // hitting right side of block
									   : pos.x - 0.5f + ((1 - m_player_size().x) / 2.f);   // hitting left side of block
				x_collided		 = true;
				// if we're walking into a moving platform moving away from us, then we want to follow it, not bounce off it
				std::optional<moving_tile> walking_into = m_moving_platform_handle[int(cx > pos.x ? dir::left : dir::right)];
				if (walking_into && util::same_sign(walking_into->vel().x, m_xv) && std::abs(m_xv) > 0.01f) {
					m_xv = walking_into->vel().x;
				} else {
					m_xv = 0;
				}
				cx = intended_x;
			}
		}

		if (!y_collided) {
			// y first
			cy = util::lerp(initial_y, intended_y, t);

			// check y collision
			sf::FloatRect aabb	  = m_get_player_y_aabb(cx, cy);
			auto collided_static  = m_tmap.intersects(aabb);
			auto collided_dynamic = m_mt_mgr.intersects(aabb);
			auto collided		  = util::merge_contacts(collided_static, collided_dynamic);

			// if colliding, disable velocity in that direction, stop checking for collision,
			// and set the position to the edge of the block
			if (m_handle_contact(cx, cy, collided)) {
				// retrieve the first solid collision
				auto [pos, tile] = m_first_solid(collided);
				intended_y		 = cy > pos.y
									   ? pos.y + 1.5f - ((1 - m_player_size().y) / 2.f)	   // hitting bottom side of block
									   : pos.y - 0.5f + ((1 - m_player_size().y) / 2.f);   // hitting top side of block
				cy				 = intended_y;
				y_collided		 = true;
				m_yv			 = 0;
			}
		}
	}


	m_xp = intended_x;
	m_yp = intended_y;

	/*static bool arr[4] = { false, false, false, false };
	for (int i = 0; i < 4; ++i) {
		if (!!m_moving_platform_handle[i] != arr[i]) {
			debug::log() << "mp u="
						 << !!m_moving_platform_handle[0]
						 << " d="
						 << !!m_moving_platform_handle[2]
						 << " r="
						 << !!m_moving_platform_handle[1]
						 << " l="
						 << !!m_moving_platform_handle[3]
						 << "\n";
			debug::log() << "touching Y-: " << m_touching[int(dir::up)] << "\n";
			debug::log() << "touching Y+: " << m_touching[int(dir::down)] << "\n";
			debug::log() << "touching X-: " << m_touching[int(dir::left)] << "\n";
			debug::log() << "touching X+: " << m_touching[int(dir::right)] << "\n";
			debug::log() << "-----------------------------"
						 << "\n";
			break;
		}
	}
	for (int i = 0; i < 4; ++i) {
		arr[i] = !!m_moving_platform_handle[i];
	}*/

	// check if we're on a moving platform
	m_update_mp();
	// update "touching" list
	m_update_touching();

	// test for being squeezed
	if (m_player_is_squeezed()) {
		m_player_die();
	}

	// handle gravity blocks
	if (m_test_touching_any(m_flip_gravity ? dir::up : dir::down, [](tile t) { return t == tile::gravity; })) {
		m_r.play_sound("gravityflip");
		m_flip_gravity = !m_flip_gravity;
		m_player.setScale(m_player.getScale().x, m_flip_gravity ? -1 : 1);
	}
	// some debug info
	debug::get() << "velocity = " << sf::Vector2f(m_xv, m_yv) << "\n";
	debug::get() << "touching Y-: " << m_touching[int(dir::up)] << "\n";
	debug::get() << "touching Y+: " << m_touching[int(dir::down)] << "\n";
	debug::get() << "touching X-: " << m_touching[int(dir::left)] << "\n";
	debug::get() << "touching X+: " << m_touching[int(dir::right)] << "\n";

	debug::get() << "mp u="
				 << !!m_moving_platform_handle[0]
				 << " d="
				 << !!m_moving_platform_handle[2]
				 << " r="
				 << !!m_moving_platform_handle[1]
				 << " l="
				 << !!m_moving_platform_handle[3]
				 << "\n";

	// update the player's animation
	m_update_animation();
	m_sync_player_position();
}

bool world::m_handle_contact(float x, float y, std::vector<std::pair<sf::Vector2f, tile>> contacts) {
	if (contacts.size() == 0) return false;
	bool touching_solid	  = false;
	bool touching_harmful = false;
	for (auto& [pos, tile] : contacts) {
		if (tile.solid()) {
			touching_solid = true;
		}
		if (tile.harmful()) {
			touching_harmful = true;
		}
	}

	if (touching_harmful) {
		if (!touching_solid) {
			m_player_die();
			return false;
		} else {
			return true;
		}
	} else {
		return touching_solid;
	}
}

std::pair<sf::Vector2f, tile> world::m_first_solid(std::vector<std::pair<sf::Vector2f, tile>> contacts) {
	for (auto& pair : contacts) {
		if (pair.second.solid()) {
			return pair;
		}
	}
	throw "no solid collision!";
}

void world::m_update_touching() {
	for (int i = 0; i < 4; ++i) {
		m_touching[i].clear();
		sf::FloatRect aabb = m_get_player_ghost_aabb(m_xp, m_yp, dir(i));
		auto contacts	   = m_tmap.intersects(aabb);
		if (contacts.size() != 0) {
			for (auto& [pos, tile] : contacts) {
				m_touching[i].push_back(tile);
			}
		}
		// add the moving tile
		if (m_moving_platform_handle[i]) {
			m_touching[i].push_back(tile(*m_moving_platform_handle[i]));
		}
	}
}

void world::m_update_mp() {
	for (int i = 0; i < 4; ++i) {
		m_moving_platform_handle[i].reset();
		sf::FloatRect aabb = m_get_player_ghost_aabb(m_xp, m_yp, dir(i));
		auto intersects	   = m_mt_mgr.intersects_raw(aabb);
		if (intersects.size() != 0) {
			m_moving_platform_handle[i].emplace(intersects.begin()->second);
		}
	}
}

sf::Vector2f world::m_mp_player_offset(sf::Time dt) const {
	sf::Vector2f offset(0, 0);

	// check the one we're standing on first
	std::optional<moving_tile> standing_on = m_moving_platform_handle[int(m_flip_gravity ? dir::up : dir::down)];
	if (standing_on) {
		offset.x += standing_on->vel().x * dt.asSeconds();
		offset.y += standing_on->vel().y * dt.asSeconds();
	}

	// check left / right moving platforms
	std::optional<moving_tile> left	 = m_moving_platform_handle[int(dir::left)];
	std::optional<moving_tile> right = m_moving_platform_handle[int(dir::right)];
	// if they're moving towards us, push the character
	if (left && left->vel().x > 0) {
		offset.x += left->vel().x * dt.asSeconds();
	}
	if (right && right->vel().x < 0) {
		offset.x += right->vel().x * dt.asSeconds();
	}


	return offset;
}

void world::m_update_animation() {
	if (std::abs(m_xv) > 0.3f) {
		m_player.set_animation("walk");
	} else {
		m_player.set_animation("stand");
	}

	// jumping
	if (std::abs(m_yv) > 0.01f) {
		if (m_yv < phys.yv_max * 0.5f) {
			m_player.set_animation("jump");
		} else if (m_yv > phys.yv_max * 0.5f) {
			m_player.set_animation("fall");
		}
	}
}

bool world::m_player_is_squeezed() {
	auto solid = [](tile t) { return t.solid(); };

	// these strange if-statements are purposeful to allow short-circuiting of this expensive operation
	bool touching_left_static = (!m_moving_platform_handle[dir::left] || m_moving_platform_handle[dir::left]->vel().x == 0) &&	 //
								m_test_touching_any(dir::left, solid);
	bool touching_right_static = (!m_moving_platform_handle[dir::right] || m_moving_platform_handle[dir::right]->vel().x == 0) &&	//
								 m_test_touching_any(dir::right, solid);
	bool touching_left_dynamic	= m_moving_platform_handle[dir::left] && m_moving_platform_handle[dir::left]->vel().x > 0;
	bool touching_right_dynamic = m_moving_platform_handle[dir::right] && m_moving_platform_handle[dir::right]->vel().x < 0;
	if ((touching_left_static && touching_right_dynamic) ||	  //
		(touching_left_dynamic && touching_right_static) ||	  //
		(touching_left_dynamic && touching_right_dynamic)) {
		return true;
	}
	// y-axis is special in that we're touching the ceiling when standing below one, so we test for moving platforms
	bool touching_up_static = (!m_moving_platform_handle[dir::up] || m_moving_platform_handle[dir::up]->vel().y == 0) &&   //
							  m_test_touching_any(dir::up, solid);
	bool touching_down_static = (!m_moving_platform_handle[dir::down] || m_moving_platform_handle[dir::down]->vel().y == 0) &&	 //
								m_test_touching_any(dir::down, solid);
	bool touching_up_dynamic   = m_moving_platform_handle[dir::up] && m_moving_platform_handle[dir::up]->vel().y > 0;
	bool touching_down_dynamic = m_moving_platform_handle[dir::down] && m_moving_platform_handle[dir::down]->vel().y < 0;
	return (touching_up_static && touching_down_dynamic)	  //
		   || (touching_up_dynamic && touching_down_static)	  //
		   || (touching_up_dynamic && touching_down_dynamic);
}

void world::draw(sf::RenderTarget& t, sf::RenderStates s) const {
	s.transform *= getTransform();
	t.draw(m_tmap, s);
	t.draw(m_mt_mgr, s);
	t.draw(m_player, s);
}

void world::m_player_die() {
	debug::log() << "death report:\n";
	debug::log() << "velocity = " << sf::Vector2f(m_xv, m_yv) << "\n";
	debug::log() << "touching Y-: " << m_touching[int(dir::up)] << "\n";
	debug::log() << "touching Y+: " << m_touching[int(dir::down)] << "\n";
	debug::log() << "touching X-: " << m_touching[int(dir::left)] << "\n";
	debug::log() << "touching X+: " << m_touching[int(dir::right)] << "\n";
	debug::log() << "mp u="
				 << !!m_moving_platform_handle[0]
				 << " d="
				 << !!m_moving_platform_handle[2]
				 << " l="
				 << !!m_moving_platform_handle[1]
				 << " r="
				 << !!m_moving_platform_handle[3]
				 << "\n";
	m_dead = true;
	m_r.play_sound("gameover");
}

void world::m_sync_player_position() {
	m_player.setPosition(m_xp * m_player.size().x, m_yp * m_player.size().y);
}

bool world::m_player_grounded() const {
	return m_test_touching_any(m_flip_gravity ? dir::up : dir::down, [](tile t) { return t.solid(); });
}

bool world::m_tile_above_player() const {
	return m_test_touching_any(m_flip_gravity ? dir::down : dir::up, [](tile t) { return t.solid(); });
}

sf::Vector2f world::m_player_size() const {
	return { 0.825f, 0.9f };
}

sf::FloatRect world::m_get_player_aabb(float x, float y) const {
	// aabb will be the sprite's full hitbox, minus 3 px on the x axis
	sf::FloatRect ret;
	ret.left   = x - 0.5f + ((1 - m_player_size().x) / 2.f);
	ret.top	   = y - 0.5f + ((1 - m_player_size().y) / 2.f);
	ret.width  = m_player_size().x;
	ret.height = m_player_size().y;

	return ret;
}

sf::FloatRect world::m_get_player_aabb(float x, float y, dir d) const {
	switch (d) {
	case up: return m_get_player_top_aabb(x, y);
	case right: return m_get_player_right_aabb(x, y);
	case down: return m_get_player_bottom_aabb(x, y);
	case left: return m_get_player_left_aabb(x, y);
	}
}

sf::FloatRect world::m_get_player_top_aabb(float x, float y) const {
	sf::FloatRect aabb = m_get_player_aabb(x, y);
	sf::FloatRect ret;
	ret.left   = aabb.left + 0.1f;
	ret.top	   = aabb.top;
	ret.width  = aabb.width - 0.2f;
	ret.height = 0.1f;

	return ret;
}

sf::FloatRect world::m_get_player_bottom_aabb(float x, float y) const {
	sf::FloatRect aabb = m_get_player_aabb(x, y);
	sf::FloatRect ret;
	ret.left   = aabb.left + 0.1f;
	ret.top	   = aabb.top + aabb.height - 0.1f;
	ret.width  = aabb.width - 0.2f;
	ret.height = 0.1f;

	return ret;
}

sf::FloatRect world::m_get_player_left_aabb(float x, float y) const {
	sf::FloatRect aabb = m_get_player_aabb(x, y);
	sf::FloatRect ret;
	ret.left   = aabb.left;
	ret.top	   = aabb.top + 0.1f;
	ret.width  = 0.1f;
	ret.height = aabb.height - 0.2f;

	return ret;
}

sf::FloatRect world::m_get_player_right_aabb(float x, float y) const {
	sf::FloatRect aabb = m_get_player_aabb(x, y);
	sf::FloatRect ret;
	ret.left   = aabb.left + aabb.width - 0.1f;
	ret.top	   = aabb.top + 0.1f;
	ret.width  = 0.1f;
	ret.height = aabb.height - 0.2f;

	return ret;
}

sf::FloatRect world::m_get_player_x_aabb(float x, float y) const {
	sf::FloatRect aabb = m_get_player_aabb(x, y);
	sf::FloatRect ret;
	ret.left   = aabb.left;
	ret.top	   = aabb.top + 0.1f;
	ret.width  = aabb.width;
	ret.height = aabb.height - 0.2f;

	return ret;
}

sf::FloatRect world::m_get_player_y_aabb(float x, float y) const {
	sf::FloatRect aabb = m_get_player_aabb(x, y);
	sf::FloatRect ret;
	ret.left   = aabb.left + 0.1f;
	ret.top	   = aabb.top;
	ret.width  = aabb.width - 0.2f;
	ret.height = aabb.height;

	return ret;
}

sf::FloatRect world::m_get_player_ghost_aabb(float x, float y, dir d) const {
	switch (d) {
	case up: return m_get_player_top_ghost_aabb(x, y);
	case right: return m_get_player_right_ghost_aabb(x, y);
	case down: return m_get_player_bottom_ghost_aabb(x, y);
	case left: return m_get_player_left_ghost_aabb(x, y);
	}
}

sf::FloatRect world::m_get_player_top_ghost_aabb(float x, float y) const {
	sf::FloatRect aabb = m_get_player_top_aabb(x, y);
	aabb.top -= 0.15f;
	return aabb;
}

sf::FloatRect world::m_get_player_bottom_ghost_aabb(float x, float y) const {
	sf::FloatRect aabb = m_get_player_bottom_aabb(x, y);
	aabb.top += 0.15f;
	return aabb;
}

sf::FloatRect world::m_get_player_left_ghost_aabb(float x, float y) const {
	sf::FloatRect aabb = m_get_player_left_aabb(x, y);
	aabb.left -= 0.05f;
	return aabb;
}

sf::FloatRect world::m_get_player_right_ghost_aabb(float x, float y) const {
	sf::FloatRect aabb = m_get_player_right_aabb(x, y);
	aabb.left += 0.05f;
	return aabb;
}

sf::FloatRect world::m_get_player_x_ghost_aabb(float x, float y) const {
	sf::FloatRect aabb = m_get_player_x_aabb(x, y);
	aabb.left -= 0.05f;
	aabb.width += 0.1f;
	return aabb;
}

sf::FloatRect world::m_get_player_y_ghost_aabb(float x, float y) const {
	sf::FloatRect aabb = m_get_player_y_aabb(x, y);
	aabb.top -= 0.15f;
	aabb.height += 0.3f;
	return aabb;
}

bool world::m_test_touching_all(dir d, std::function<bool(tile)> pred) const {
	if (m_touching[int(d)].size() == 0) return false;
	return std::all_of(m_touching[int(d)].begin(), m_touching[int(d)].end(), pred);
}

bool world::m_test_touching_any(dir d, std::function<bool(tile)> pred) const {
	if (m_touching[int(d)].size() == 0) return false;
	return std::any_of(m_touching[int(d)].begin(), m_touching[int(d)].end(), pred);
}

bool world::m_test_touching_none(dir d, std::function<bool(tile)> pred) const {
	if (m_touching[int(d)].size() == 0) return false;
	return std::none_of(m_touching[int(d)].begin(), m_touching[int(d)].end(), pred);
}
