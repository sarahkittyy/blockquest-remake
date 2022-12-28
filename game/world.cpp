#include "world.hpp"

#include "debug.hpp"
#include "imgui_internal.h"
#include "util.hpp"

#include "particles/death.hpp"
#include "particles/gravity.hpp"
#include "particles/smoke.hpp"
#include "particles/victory.hpp"

#include "resource.hpp"

world::world(level l, std::optional<replay> rp)
	: m_has_focus(true),
	  m_tmap(l.map()),
	  m_mt_mgr(m_tmap),
	  m_level(l),
	  m_player(resource::get().tex("assets/player.png")),
	  m_start_x(0),
	  m_start_y(0),
	  m_cstep(0),
	  m_playback(rp),
	  m_flip_gravity(false),
	  m_moving_platform_handle() {
	m_player.set_animation("walk");
	m_player.setOrigin(m_player.size().x / 2.f, m_player.size().y / 2.f);
	m_init_world();
	m_pmgr.setScale(l.map().tile_size(), l.map().tile_size());

	m_game_clear.setTexture(resource::get().tex("assets/gui/victory.png"));
	m_space_to_retry.setTexture(resource::get().tex("assets/gui/retry.png"));
	m_fadeout.setFillColor(sf::Color(127, 127, 127, 0));
	m_fadeout.setSize(m_tmap.total_size());
	m_game_clear.setOrigin(m_game_clear.getLocalBounds().width / 2.f, m_game_clear.getLocalBounds().height);
	m_space_to_retry.setOrigin(m_space_to_retry.getLocalBounds().width / 2.f, 0);

	m_game_over.setTexture(resource::get().tex("assets/gui/defeat.png"));
	m_game_over.setOrigin(m_game_over.getLocalBounds().width / 2.f, m_game_over.getLocalBounds().height);
	m_fadeout.setOrigin(m_fadeout.getLocalBounds().width / 2.f, m_fadeout.getLocalBounds().height / 2.f);

	m_game_clear.setScale(4, 4);
	m_space_to_retry.setScale(4, 4);
	m_game_over.setScale(4, 4);
	m_fadeout.setScale(4, 4);

	m_game_clear.setPosition(m_tmap.total_size() / 2.f);
	m_space_to_retry.setPosition(m_tmap.total_size() / 2.f);
	m_game_over.setPosition(m_tmap.total_size() / 2.f);
	m_fadeout.setPosition(m_tmap.total_size() / 2.f);

	m_dash_sfx_thread = std::jthread([this](std::stop_token stoken) {
		using namespace std::chrono_literals;
		sf::Sound s(resource::get().sound_buffer("dash"));
		while (!stoken.stop_requested()) {
			if (m_dashing && m_player_grounded() && std::abs(m_xv) > phys.xv_max) {
				s.play();
				auto& sp		= m_pmgr.spawn<particles::smoke>();
				float grav_sign = m_flip_gravity ? -1 : 1;
				sp.setPosition(m_xp, m_yp + 0.2f * grav_sign);
				sp.setScale(m_dash_dir == dir::right ? -1 : 1, grav_sign);
			}
			std::this_thread::sleep_for(120ms);
		}
	});
}

world::~world() {
	m_dash_sfx_thread.request_stop();
	m_dash_sfx_thread.join();
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
	m_yp		   = m_start_y + 0.499f;
	m_xv		   = 0;
	m_yv		   = 0;
	m_flip_gravity = false;
	m_dead		   = false;
	m_sync_player_position();
	m_player.setScale(1, 1);
	m_mt_mgr.restart();
	m_time_airborne	 = sf::seconds(999);
	m_jumping		 = true;
	m_dashing		 = false;
	m_since_wallkick = sf::seconds(999);
	m_this_frame	 = input_state();
	m_last_frame	 = input_state();
	m_climbing		 = false;
	m_touched_goal	 = false;
	m_ctime			 = sf::Time::Zero;
	m_replay.reset();
	m_cstep = 0;
	m_fadeout.setFillColor(sf::Color(0, 0, 0, 0));
}

bool world::won() const {
	return m_touched_goal;
}

bool world::lost() const {
	return m_dead;
}

bool world::has_playback() const {
	return m_playback.has_value();
}

replay& world::get_replay() {
	return m_playback.has_value() ? *m_playback : m_replay;
}

void world::process_event(sf::Event e) {
	switch (e.type) {
	default:
		break;
	case sf::Event::LostFocus:
		m_has_focus = false;
		break;
	case sf::Event::GainedFocus:
		m_has_focus = true;
		break;
	}
}

void world::step(sf::Time dt) {
	// -1 if gravity is flipped. used for y velocity calculations
	float gravity_sign = m_flip_gravity ? -1 : 1;

	m_cstep++;

	if (!m_playback && !m_dead) m_replay.push(m_this_frame);

	// update moving platforms
	m_mt_mgr.update(dt);

	// check if we're on a moving platform
	m_update_mp();
	// update "touching" list
	m_update_touching();

	// shift the player by the amount the platform they're on moved
	sf::Vector2f mp_offset = m_mp_player_offset(dt);
	m_xp += mp_offset.x;
	m_yp += mp_offset.y;

	bool grounded = m_player_grounded();
	if (grounded) {
		m_time_airborne = sf::seconds(0);
	} else {
		m_time_airborne += dt;
	}

	m_since_wallkick += dt;

	// controls //

	if (m_this_frame.dash) {
		// can only start dashing if on the ground
		if (grounded && !m_climbing) {
			if (!m_dashing) {	// start of dash
				m_dash_dir = m_facing();
			}
			m_dashing = true;
		}
	} else if (grounded) {
		m_dashing = false;
	}

	float air_control_factor	  = grounded ? 1 : (m_dashing && m_this_frame.dash ? phys.dash_air_control : phys.air_control);
	float ground_control_factor	  = m_dashing && grounded ? 0 : 1;
	float wallkick_control_factor = m_is_wallkick_locked() ? 0 : 1;
	bool on_ice					  = m_on_ice();	  // since this is expensive
	float friction_control_factor = on_ice && grounded ? phys.ice_friction : 1;
	if (m_dashing) {
		// if dashing, l/r controls are disabled and we accelerate at full speed in the dash direction
		float xv_sign = m_dash_dir == dir::left ? -1 : 1;
		if (grounded)
			m_xv += phys.dash_x_accel * dt.asSeconds() *
					air_control_factor *
					friction_control_factor *
					xv_sign;
		else
			m_dash_dir = m_facing();
	}
	// disables climbing if we're no longer against a ladder
	if (m_climbing && !m_against_ladder(m_climbing_facing)) {
		m_climbing = false;
		// if we're going "up"
		if (m_yv * gravity_sign < 0 && !m_is_wallkick_locked()) {
			m_xv = phys.climb_dismount_xv * (m_climbing_facing == dir::left ? -1 : 1);
		}
	}
	bool lr_inputted = false;
	if (m_this_frame.right) {
		lr_inputted = !lr_inputted;
		// wallkick
		if (m_can_player_wallkick(dir::left)) {
			m_player_wallkick(dir::left);
		} else if (!m_climbing && m_against_ladder(dir::right)) {
			m_climbing		  = true;
			m_climbing_facing = dir::right;
			m_yv			  = 0;
		} else if (m_climbing && m_against_ladder(dir::left) && m_last_frame.right) {
			m_climbing = false;
		} else if (!m_this_frame.left) {
			// normal acceleration
			if (m_xv < 0 && !on_ice) {
				m_xv += phys.x_decel * dt.asSeconds() *
						air_control_factor *
						ground_control_factor *
						wallkick_control_factor *
						friction_control_factor;
			}
			m_xv += phys.x_accel * dt.asSeconds() *
					air_control_factor *
					ground_control_factor *
					wallkick_control_factor *
					friction_control_factor;
			// we can only change direction when not dashing
			if ((!m_dashing || !grounded) && !m_is_wallkick_locked())
				m_player.setScale(-1, m_player.getScale().y);
		}
	}
	if (m_this_frame.left) {
		lr_inputted = !lr_inputted;
		// wallkick
		if (m_can_player_wallkick(dir::right)) {
			m_player_wallkick(dir::right);
		} else if (!m_climbing && m_against_ladder(dir::left)) {
			m_climbing		  = true;
			m_climbing_facing = dir::left;
			m_yv			  = 0;
		} else if (m_climbing && m_against_ladder(dir::right) && m_last_frame.right) {
			m_climbing = false;
		} else if (!m_this_frame.right) {
			// normal acceleration
			if (m_xv > 0 && !on_ice) {
				m_xv -= phys.x_decel * dt.asSeconds() *
						air_control_factor *
						ground_control_factor *
						wallkick_control_factor;
			}
			m_xv -= phys.x_accel * dt.asSeconds() *
					air_control_factor *
					ground_control_factor *
					wallkick_control_factor *
					friction_control_factor;
			// we can only change direction when not dashing
			if ((!m_dashing || !grounded) && !m_is_wallkick_locked())
				m_player.setScale(1, m_player.getScale().y);
		}
	}
	if (!lr_inputted && !m_dashing && !m_is_wallkick_locked()) {
		if (m_xv > (phys.x_decel / 2.f) * dt.asSeconds()) {
			m_xv -= phys.x_decel *
					friction_control_factor *
					dt.asSeconds();
		} else if (m_xv < (-phys.x_decel / 2.f) * dt.asSeconds()) {
			m_xv += phys.x_decel *
					friction_control_factor *
					dt.asSeconds();
		} else {
			m_xv = 0;
		}
	}

	if (m_this_frame.jump) {
		if (!m_jumping && !m_climbing && m_player_grounded_ago(sf::milliseconds(phys.coyote_millis)) && !m_tile_above_player()) {
			m_yv = -phys.jump_v * gravity_sign;
			// so that we can't jump twice :)
			m_time_airborne = sf::seconds(999);
			m_jumping		= true;
			resource::get().play_sound("jump");
			// to prevent sticking
			m_yp -= 0.01f * gravity_sign;
		} else if (m_climbing && !m_last_frame.jump && m_can_player_wallkick(mirror(m_climbing_facing), false)) {
			m_player_wallkick(mirror(m_climbing_facing));
		}
	} else {
		if (m_jumping) {
			m_jumping = false;
			if (!m_is_wallkick_locked())
				m_yv *= phys.shorthop_factor;
		}
	}

	if (m_climbing) {	// up and down controls while climbing
		if (m_this_frame.up) {
			m_yv -= phys.climb_ya * dt.asSeconds() * gravity_sign;
			// to prevent sticking
			if (m_player_grounded())
				m_yp -= 0.01f * gravity_sign;
		} else if (m_this_frame.down) {
			m_yv += phys.climb_ya * dt.asSeconds() * gravity_sign;
		} else {
			if (m_yv > (phys.climb_ya / 2.f) * dt.asSeconds()) {
				m_yv -= phys.climb_ya * dt.asSeconds();
			} else if (m_yv < -(phys.climb_ya / 2.f) * dt.asSeconds()) {
				m_yv += phys.climb_ya * dt.asSeconds();
			} else {
				m_yv = 0;
			}
		}
		m_yv = util::clamp(m_yv, -phys.climb_yv_max, phys.climb_yv_max);
	}
	/////////////////////

	float real_xv_max = m_dashing ? phys.dash_xv_max : phys.xv_max;
	m_xv			  = util::clamp(m_xv, -real_xv_max, real_xv_max);

	if (!m_climbing) {	 // no gravity while climbing
		m_yv += phys.grav * dt.asSeconds() * gravity_sign;
		if (m_flip_gravity) {
			m_yv = util::clamp(m_yv, -phys.yv_max, phys.jump_v);
		} else {
			m_yv = util::clamp(m_yv, -phys.jump_v, phys.yv_max);
		}
	}

	// !! physics !! //

	float initial_x	 = m_xp;
	float initial_y	 = m_yp;
	float intended_x = m_xp + m_xv * dt.asSeconds();
	float intended_y = m_yp + m_yv * dt.asSeconds();

	bool x_collided = false;
	bool y_collided = false;

	float cx = initial_x, cy = initial_y;

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
				// edge case to solve a bug in which moving against a bottom-right corner would trap you
				if (cx < pos.x)
					intended_x -= m_xv < 0 ? 0.01f : 0;
				else if (cx > pos.x)
					intended_x += m_xv > 0 ? 0.01f : 0;
				// if we're walking into a moving platform moving away from us, then we want to follow it, not bounce off it
				std::optional<moving_tile> walking_into = m_moving_platform_handle[int(cx > pos.x ? dir::left : dir::right)];
				if (walking_into &&
					util::same_sign(walking_into->vel().x, m_xv) &&
					std::abs(m_xv) > 0.01f) {
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
									   ? pos.y + 1.5f
									   : pos.y - 0.5f + ((1 - m_player_size().y) / 6.0f);	// hitting top side of block
				cy				 = intended_y;
				y_collided		 = true;
				m_yv			 = 0;
			}
		}
	}


	m_xp = intended_x;
	m_yp = intended_y;

	// test for being squeezed
	if (m_player_is_squeezed() || m_player_oob()) {
		m_player_die();
	}

	// handle gravity blocks
	if (m_test_touching_any(m_flip_gravity ? dir::up : dir::down, [](tile t) { return t == tile::gravity; })) {
		// gravity particles
		auto& gp = m_pmgr.spawn<particles::gravity>(m_flip_gravity);
		for (auto& tile : m_touching[m_flip_gravity ? dir::up : dir::down]) {
			if (tile == tile::gravity) {
				gp.setPosition(tile.x() + 0.5f, tile.y() + (m_flip_gravity ? 1 : 0));
				break;
			}
		}

		resource::get().play_sound("gravityflip");
		m_flip_gravity = !m_flip_gravity;
		m_dashing	   = false;
		m_player.setScale(m_player.getScale().x, m_flip_gravity ? -1 : 1);
	}

	// update last frame keys
	m_last_frame = m_this_frame;
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

bool world::update(sf::Time dt) {
	if (m_playback.has_value() && m_cstep < m_playback->size() && !lost() && !won()) {
		m_this_frame = m_playback->get(m_cstep);
	} else {
		m_this_frame.left  = m_has_focus ? settings::get().key_down(key::LEFT) : false;
		m_this_frame.right = m_has_focus ? settings::get().key_down(key::RIGHT) : false;
		m_this_frame.dash  = m_has_focus ? settings::get().key_down(key::DASH) : false;
		m_this_frame.jump  = m_has_focus ? settings::get().key_down(key::JUMP) : false;
		m_this_frame.up	   = m_has_focus ? settings::get().key_down(key::UP) : false;
		m_this_frame.down  = m_has_focus ? settings::get().key_down(key::DOWN) : false;
	}

	if (settings::get().key_down(key::RESTART) && !ImGui::GetIO().WantCaptureKeyboard) {
		if (!m_restarted)
			m_restart_world();
		m_restarted = true;
	} else {
		m_restarted = false;
	}

	m_pmgr.update(dt);

	// update the player's animations
	m_player.update();

	if (won()) {
		sf::Color opacity = m_space_to_retry.getColor();
		m_end_alpha += 255.f * dt.asSeconds();
		m_end_alpha = std::min(255.f, m_end_alpha);
		opacity.a	= m_end_alpha;
		m_space_to_retry.setColor(opacity);
		m_game_clear.setColor(opacity);
		m_fadeout.setFillColor(sf::Color(220, 220, 220, m_end_alpha / 2.f));
		if (m_just_jumped() && !ImGui::GetIO().WantCaptureKeyboard) {
			m_restart_world();
			return false;
		} else {
			m_last_frame.jump = m_this_frame.jump;
			return false;
		}
	} else if (lost() && !ImGui::GetIO().WantCaptureKeyboard) {
		sf::Color opacity = m_space_to_retry.getColor();
		m_end_alpha += 255.f * dt.asSeconds();
		m_end_alpha = std::min(255.f, m_end_alpha);
		opacity.a	= m_end_alpha;
		m_space_to_retry.setColor(opacity);
		m_game_over.setColor(opacity);
		m_fadeout.setFillColor(sf::Color(0, 0, 0, m_end_alpha / 2.f));
		if (m_just_jumped()) {
			m_restart_world();
			return false;
		} else {
			m_last_frame.jump = m_this_frame.jump;
			return false;
		}
	}

	// physics updates!
	m_ctime += dt;
	bool stepped = false;
	while (m_ctime > replay::timestep) {
		m_ctime -= replay::timestep;
		step(replay::timestep);
		stepped = true;
	}

	// debug::get().box(util::scale<float>(m_get_player_top_aabb(m_xp, m_yp), m_player.size().x));
	// debug::get().box(util::scale<float>(m_get_player_bottom_aabb(m_xp, m_yp), m_player.size().x));
	//  debug::get().box(util::scale<float>(m_get_player_left_aabb(m_xp, m_yp), m_player.size().x));
	//  debug::get().box(util::scale<float>(m_get_player_right_aabb(m_xp, m_yp), m_player.size().x));
	//  debug::get().box(util::scale<float>(m_get_player_top_ghost_aabb(m_xp, m_yp), m_player.size().x));
	//  debug::get().box(util::scale<float>(m_get_player_bottom_ghost_aabb(m_xp, m_yp), m_player.size().x));
	//  debug::get().box(util::scale<float>(m_get_player_left_ghost_aabb(m_xp, m_yp), m_player.size().x));
	//  debug::get().box(util::scale<float>(m_get_player_right_ghost_aabb(m_xp, m_yp), m_player.size().x));
	//  debug::get().box(util::scale<float>(m_get_player_aabb(m_xp, m_yp), m_player.size().x));
	//  debug::get().box(util::scale<float>(m_get_player_x_aabb(m_xp, m_yp), m_player.size().x));
	debug::get().box(util::scale<float>(m_get_player_y_ghost_aabb(m_xp, m_yp), m_player.size().x));


	debug::get() << "dashing = " << m_dashing << "\n";
	debug::get() << "airborne = " << m_time_airborne.asSeconds() << "\n";
	debug::get() << "climbing = " << m_climbing << "\n";
	debug::get() << "won = " << won() << "\n";

	// some debug info
	debug::get() << "dt = " << dt.asMilliseconds() << "ms\n";
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

	return stepped;
}

void world::m_player_wallkick(dir d) {
	if (m_is_wallkick_locked()) return;
	float xv_sign	= d == dir::left ? -1 : 1;
	float grav_sign = m_flip_gravity ? -1 : 1;
	m_climbing		= false;
	m_xv			= phys.wallkick_xv * xv_sign;
	m_yv			= -phys.wallkick_yv * grav_sign;
	m_xp += ((1 - m_player_size().x) / 2.f) * xv_sign;
	auto& sp = m_pmgr.spawn<particles::smoke>();
	sp.setPosition(m_xp - 0.35f * xv_sign, m_yp);
	sp.setScale(xv_sign, sp.getScale().y);
	resource::get().play_sound("wallkick");
	m_player.setScale(-xv_sign, m_player.getScale().y);
	m_since_wallkick = sf::Time::Zero;
}

bool world::m_is_wallkick_locked() const {
	return m_since_wallkick < sf::milliseconds(200);
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
		if (tile == tile::end) {
			m_player_win();
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
		// save the first solid tile
		for (auto& [pos, mt] : intersects) {
			if (tile(mt).solid()) {
				m_moving_platform_handle[i].emplace(mt);
				break;
			}
		}
	}
}

sf::Vector2f world::m_mp_player_offset(sf::Time dt) const {
	sf::Vector2f offset(0, 0);

	// check the one we're standing on first
	std::optional<moving_tile> standing_on = m_moving_platform_handle[int(m_flip_gravity ? dir::up : dir::down)];
	if (standing_on && tile(*standing_on).solid()) {
		if (tile(*standing_on) != tile::ice) {
			offset.x += standing_on->delta().x;
		}
		offset.y += standing_on->delta().y;
	}

	// check left / right moving platforms
	std::optional<moving_tile> left	 = m_moving_platform_handle[int(dir::left)];
	std::optional<moving_tile> right = m_moving_platform_handle[int(dir::right)];
	// if they're moving towards us, push the character
	if (left && tile(*left).solid() && left->vel().x > 0) {
		offset.x += left->vel().x * dt.asSeconds();
	}
	if (right && tile(*right).solid() && right->vel().x < 0) {
		offset.x += right->vel().x * dt.asSeconds();
	}
	if (left && tile(*left) == tile::ladder && m_climbing && m_climbing_facing == dir::left) {
		offset.y += left->vel().y * dt.asSeconds();
	}
	if (right && tile(*right) == tile::ladder && m_climbing && m_climbing_facing == dir::right) {
		offset.y += right->vel().y * dt.asSeconds();
	}

	// specific case for climbing on a moving ladder going away from us
	if (m_climbing) {
		if (m_climbing_facing == dir::left && left && left->vel().x < 0) {
			offset.x += left->vel().x * dt.asSeconds();
		}
		if (m_climbing_facing == dir::right && right && right->vel().x > 0) {
			offset.x += right->vel().x * dt.asSeconds();
		}
	}

	return offset;
}

void world::m_update_animation() {
	if (m_climbing) {
		if (std::abs(m_yv) > 0.01f) {
			m_player.set_animation("climb");
		} else {
			m_player.set_animation("hang");
		}
		m_player.setScale(m_climbing_facing == dir::left ? 1 : -1, m_player.getScale().y);
	} else if (std::abs(m_xv) > phys.xv_max) {
		m_player.set_animation("dash");
	} else if (std::abs(m_xv) > 0.3f) {
		if (m_on_ice()) {
			if (m_this_frame.left || m_this_frame.right || m_this_frame.dash) {
				m_player.set_animation("walk");
			} else {
				m_player.set_animation("stand");
			}
		} else {
			m_player.set_animation("walk");
		}
	} else {
		m_player.set_animation("stand");
	}

	// jumping
	if (!m_climbing && std::abs(m_yv) > 0) {
		if (m_flip_gravity) {
			if (m_yv > -phys.yv_max * 0.2f) {
				m_player.set_animation("jump");
			} else if (m_yv < -phys.yv_max * 0.5f) {
				m_player.set_animation("fall");
			}
		} else {
			if (m_yv < phys.yv_max * 0.2f) {
				m_player.set_animation("jump");
			} else if (m_yv > phys.yv_max * 0.5f) {
				m_player.set_animation("fall");
			}
		}
	}
}

bool world::m_player_is_squeezed() {
	auto solid = [](tile t) { return t.solid(); };

	// these strange if-statements are purposeful to allow short-circuiting of this expensive operation
	bool touching_left_static = (!m_moving_platform_handle[int(dir::left)] || m_moving_platform_handle[int(dir::left)]->vel().x == 0) &&   //
								m_test_touching_any(dir::left, solid);
	bool touching_right_static = (!m_moving_platform_handle[int(dir::right)] || m_moving_platform_handle[int(dir::right)]->vel().x == 0) &&	  //
								 m_test_touching_any(dir::right, solid);
	bool touching_left_dynamic	= m_moving_platform_handle[int(dir::left)] && m_moving_platform_handle[int(dir::left)]->vel().x > 0;
	bool touching_right_dynamic = m_moving_platform_handle[int(dir::right)] && m_moving_platform_handle[int(dir::right)]->vel().x < 0;
	if ((touching_left_static && touching_right_dynamic) ||	  //
		(touching_left_dynamic && touching_right_static) ||	  //
		(touching_left_dynamic && touching_right_dynamic)) {
		return true;
	}
	// y-axis is special in that we're touching the ceiling when standing below one, so we test for moving platforms
	bool touching_up_static = (!m_moving_platform_handle[int(dir::up)] || m_moving_platform_handle[int(dir::up)]->vel().y == 0) &&	 //
							  m_test_touching_any(dir::up, solid);
	bool touching_down_static = (!m_moving_platform_handle[int(dir::down)] || m_moving_platform_handle[int(dir::down)]->vel().y == 0) &&   //
								m_test_touching_any(dir::down, solid);
	bool touching_up_dynamic   = m_moving_platform_handle[int(dir::up)] && m_moving_platform_handle[int(dir::up)]->vel().y > 0;
	bool touching_down_dynamic = m_moving_platform_handle[int(dir::down)] && m_moving_platform_handle[int(dir::down)]->vel().y < 0;
	// i'm doing all these individual checks to make sure the gap is small enough to be pressed,
	// as right now if a stopped stops a moving tile 1 block before collision, squishes still occur
	if (touching_up_static && touching_down_dynamic) {
		sf::FloatRect dynamic_aabb = m_moving_platform_handle[int(dir::down)]->get_aabb();
		for (auto& tile : m_touching[int(dir::up)]) {
			if (!tile.solid()) continue;
			debug::log() << tile.y() + 1 - dynamic_aabb.top << "\n";
			if (std::abs(tile.y() + 1 - dynamic_aabb.top) < 0.9f) {
				return true;
			}
		}
	}
	if (touching_up_dynamic && touching_down_static) {
		sf::FloatRect dynamic_aabb = m_moving_platform_handle[int(dir::up)]->get_aabb();
		for (auto& tile : m_touching[int(dir::down)]) {
			if (!tile.solid()) continue;
			if (std::abs(tile.y() - (dynamic_aabb.top + dynamic_aabb.height)) < 0.9f) {
				return true;
			}
		}
	}
	if (touching_up_dynamic && touching_down_dynamic) {
		sf::FloatRect dynamic_up_aabb	= m_moving_platform_handle[int(dir::up)]->get_aabb();
		sf::FloatRect dynamic_down_aabb = m_moving_platform_handle[int(dir::down)]->get_aabb();
		if (std::abs((dynamic_up_aabb.top + dynamic_up_aabb.height) - dynamic_down_aabb.top) < 0.9f) {
			return true;
		}
	}
	return false;
}

void world::draw(sf::RenderTarget& t, sf::RenderStates s) const {
	s.transform *= getTransform();
	t.draw(m_tmap, s);
	t.draw(m_mt_mgr, s);
	if (!won() && !lost())
		t.draw(m_player, s);
	t.draw(m_pmgr, s);
	if (won()) {
		t.draw(m_fadeout, s);
		t.draw(m_game_clear, s);
		t.draw(m_space_to_retry, s);
	} else if (lost()) {
		t.draw(m_fadeout, s);
		t.draw(m_game_over, s);
		t.draw(m_space_to_retry, s);
	}
}

void world::m_player_win() {
	if (m_touched_goal) return;
	m_touched_goal = true;
	m_end_alpha	   = 0;
	m_dashing	   = false;
	m_space_to_retry.setColor(sf::Color(255, 255, 255, 0));
	m_game_over.setColor(sf::Color(255, 255, 255, 0));
	m_game_clear.setColor(sf::Color(255, 255, 255, 0));
	resource::get().play_sound("victory");
	auto& sp		= m_pmgr.spawn<particles::victory>();
	float grav_sign = m_flip_gravity ? -1 : 1;
	sp.setPosition(m_xp, m_yp + 0.2f * grav_sign);
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
	resource::get().play_sound("gameover");
	m_end_alpha = 0;
	m_dashing	= false;
	m_space_to_retry.setColor(sf::Color(255, 255, 255, 0));
	m_game_over.setColor(sf::Color(255, 255, 255, 0));
	m_game_clear.setColor(sf::Color(255, 255, 255, 0));
	auto& dp = m_pmgr.spawn<particles::death>();
	dp.setPosition(m_xp, m_yp);
	dp.setScale(0.5f, 0.5f);
}

void world::m_sync_player_position() {
	float xp = m_xp + m_xv * dt_since_step();
	float yp = m_yp + m_yv * dt_since_step();
	m_player.setPosition(xp * m_player.size().x, yp * m_player.size().y);
}

bool world::m_on_ice() const {
	return m_test_touching_any(m_flip_gravity ? dir::up : dir::down, [](tile t) -> bool {
		return t == tile::ice;
	});
}

bool world::m_player_grounded() const {
	return m_test_touching_any(m_flip_gravity ? dir::up : dir::down, [](tile t) { return t.solid(); });
}

bool world::m_player_grounded_ago(sf::Time t) const {
	return m_time_airborne < t;
}

bool world::m_just_jumped() const {
	return !m_last_frame.jump && m_this_frame.jump;
}

bool world::m_player_oob() const {
	bool fell_through_bottom = !m_flip_gravity ? m_yp > m_level.map().size().y : m_yp < -1;
	return m_xp < -1 || m_xp > m_level.map().size().x || fell_through_bottom;
}

bool world::m_against_ladder(dir d) const {
	return m_test_touching_any(d, [](tile t) {
		return t == tile::ladder;
	});
}

bool world::m_can_player_wallkick(dir d, bool keys_pressed) const {
	bool keyed_last_frame = d == dir::left ? m_last_frame.left : m_last_frame.right;
	bool keyed_this_frame = d == dir::left ? m_this_frame.left : m_this_frame.right;
	bool just_keyed		  = !keyed_last_frame && keyed_this_frame;

	return (!keys_pressed || just_keyed) && !m_player_grounded() &&
		   m_test_touching_any(d == dir::left ? dir::right : dir::left, [](tile t) {
			   return t.solid() && !t.blocks_wallkicks();
		   });
}

bool world::m_tile_above_player() const {
	return m_test_touching_any(m_flip_gravity ? dir::down : dir::up, [](tile t) { return t.solid(); });
}

world::dir world::m_facing() const {
	return m_player.getScale().x < 0 ? dir::right : dir::left;
}

sf::Vector2f world::m_player_size() const {
	return { 0.6f, 0.7f };
}

sf::FloatRect world::m_get_player_aabb(float x, float y) const {
	// aabb will be the sprite's full hitbox, minus 3 px on the x axis
	sf::FloatRect ret;
	ret.left   = x - 0.5f + ((1 - m_player_size().x) / 2.f);
	ret.top	   = y - 0.5f + ((1 - m_player_size().y) / 1.2f);
	ret.width  = m_player_size().x;
	ret.height = m_player_size().y;

	if (m_flip_gravity) {
		ret.top -= ((1 - m_player_size().y) / 1.2f);
		ret.height += ((1 - m_player_size().y) / 1.2f);
	}

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
	ret.top	   = aabb.top - 0.1f;
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
	ret.left   = aabb.left + 0.001f;
	ret.top	   = aabb.top + 0.1f;
	ret.width  = aabb.width - 0.001f;
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
	aabb.top -= 0.3f;
	return aabb;
}

sf::FloatRect world::m_get_player_bottom_ghost_aabb(float x, float y) const {
	sf::FloatRect aabb = m_get_player_bottom_aabb(x, y);
	aabb.top += 0.3f;
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

constexpr world::dir world::mirror(dir d) const {
	switch (d) {
	case dir::up: return dir::down;
	case dir::down: return dir::up;
	case dir::left: return dir::right;
	case dir::right: return dir::left;
	};
}
