#include "world.hpp"

#include "context.hpp"
#include "debug.hpp"
#include "imgui_internal.h"
#include "util.hpp"

#include "particles/death.hpp"
#include "particles/gravity.hpp"
#include "particles/smoke.hpp"
#include "particles/victory.hpp"

#include "auth.hpp"
#include "resource.hpp"

const world::physics world::phys;
const world::control_vars world::control_vars::empty = {
	.xp					  = -999,
	.yp					  = 999,
	.xv					  = 0,
	.yv					  = 0,
	.sx					  = 1,
	.sy					  = 1,
	.climbing			  = false,
	.dashing			  = false,
	.jumping			  = false,
	.dash_dir			  = dir::right,
	.climbing_facing	  = dir::right,
	.since_wallkick		  = sf::seconds(999),
	.time_airborne		  = sf::seconds(999),
	.this_frame			  = input_state(),
	.last_frame			  = input_state(),
	.grounded			  = true,
	.facing				  = dir::right,
	.against_ladder_left  = false,
	.against_ladder_right = false,
	.can_wallkick_left	  = false,
	.can_wallkick_right	  = false,
	.on_ice				  = false,
	.flip_gravity		  = false,
	.alt_controls		  = false,
	.tile_above			  = false
};

sio::message::ptr world::control_vars::to_message() const {
	auto ptr							   = sio::object_message::create();
	ptr->get_map()["xp"]				   = sio::double_message::create(xp);
	ptr->get_map()["yp"]				   = sio::double_message::create(yp);
	ptr->get_map()["xv"]				   = sio::double_message::create(xv);
	ptr->get_map()["yv"]				   = sio::double_message::create(yv);
	ptr->get_map()["sx"]				   = sio::double_message::create(sx);
	ptr->get_map()["sy"]				   = sio::double_message::create(sy);
	ptr->get_map()["climbing"]			   = sio::bool_message::create(climbing);
	ptr->get_map()["dashing"]			   = sio::bool_message::create(dashing);
	ptr->get_map()["jumping"]			   = sio::bool_message::create(jumping);
	ptr->get_map()["dash_dir"]			   = sio::int_message::create(static_cast<int>(dash_dir));
	ptr->get_map()["climbing_facing"]	   = sio::int_message::create(static_cast<int>(climbing_facing));
	ptr->get_map()["since_wallkick"]	   = sio::int_message::create(since_wallkick.asMilliseconds());
	ptr->get_map()["time_airborne"]		   = sio::int_message::create(time_airborne.asMilliseconds());
	ptr->get_map()["this_frame"]		   = sio::int_message::create(int(this_frame));
	ptr->get_map()["last_frame"]		   = sio::int_message::create(int(last_frame));
	ptr->get_map()["grounded"]			   = sio::bool_message::create(grounded);
	ptr->get_map()["facing"]			   = sio::int_message::create(static_cast<int>(facing));
	ptr->get_map()["against_ladder_left"]  = sio::bool_message::create(against_ladder_left);
	ptr->get_map()["against_ladder_right"] = sio::bool_message::create(against_ladder_right);
	ptr->get_map()["can_wallkick_left"]	   = sio::bool_message::create(can_wallkick_left);
	ptr->get_map()["can_wallkick_right"]   = sio::bool_message::create(can_wallkick_right);
	ptr->get_map()["on_ice"]			   = sio::bool_message::create(on_ice);
	ptr->get_map()["flip_gravity"]		   = sio::bool_message::create(flip_gravity);
	ptr->get_map()["alt_controls"]		   = sio::bool_message::create(alt_controls);
	ptr->get_map()["tile_above"]		   = sio::bool_message::create(tile_above);
	return ptr;
}

world::control_vars world::control_vars::from_message(const sio::message::ptr& msg) {
	world::control_vars controls;
	auto& data					  = msg->get_map();
	controls.xp					  = data["xp"]->get_double();
	controls.yp					  = data["yp"]->get_double();
	controls.xv					  = data["xv"]->get_double();
	controls.yv					  = data["yv"]->get_double();
	controls.sx					  = data["sx"]->get_double();
	controls.sy					  = data["sy"]->get_double();
	controls.climbing			  = data["climbing"]->get_bool();
	controls.dashing			  = data["dashing"]->get_bool();
	controls.jumping			  = data["jumping"]->get_bool();
	controls.dash_dir			  = static_cast<dir>(data["dash_dir"]->get_int());
	controls.climbing_facing	  = static_cast<dir>(data["climbing_facing"]->get_int());
	controls.since_wallkick		  = sf::milliseconds(data["since_wallkick"]->get_int());
	controls.time_airborne		  = sf::milliseconds(data["time_airborne"]->get_int());
	controls.this_frame			  = input_state::from_int(data["this_frame"]->get_int());
	controls.last_frame			  = input_state::from_int(data["last_frame"]->get_int());
	controls.grounded			  = data["grounded"]->get_bool();
	controls.facing				  = static_cast<dir>(data["facing"]->get_int());
	controls.against_ladder_left  = data["against_ladder_left"]->get_bool();
	controls.against_ladder_right = data["against_ladder_right"]->get_bool();
	controls.can_wallkick_left	  = data["can_wallkick_left"]->get_bool();
	controls.can_wallkick_right	  = data["can_wallkick_right"]->get_bool();
	controls.on_ice				  = data["on_ice"]->get_bool();
	controls.flip_gravity		  = data["flip_gravity"]->get_bool();
	controls.alt_controls		  = data["alt_controls"]->get_bool();
	controls.tile_above			  = data["tile_above"]->get_bool();

	return controls;
}

world::world(level l, std::optional<replay> rp)
	: m_has_focus(true),
	  m_tmap(l.map()),
	  m_mt_mgr(m_tmap),
	  m_level(l),
	  m_player(),
	  m_start_x(0),
	  m_start_y(0),
	  m_cstep(0),
	  m_playback(rp),
	  m_cvars(control_vars::empty),
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
			if (m_cvars.dashing && m_player_grounded() && std::abs(m_cvars.xv) > phys.xv_max && !lost() && !won()) {
				s.setVolume(context::get().sfx_volume());
				s.play();
				auto& sp		= m_pmgr.spawn<particles::smoke>();
				float grav_sign = m_cvars.flip_gravity ? -1 : 1;
				sp.setPosition(m_cvars.xp, m_cvars.yp + 0.2f * grav_sign);
				sp.setScale(m_cvars.dash_dir == dir::right ? -1 : 1, grav_sign);
			}
			std::this_thread::sleep_for(120ms);
		}
	});
}

world::~world() {
	m_dash_sfx_thread.request_stop();
	m_dash_sfx_thread.join();
}

sf::Vector2f world::get_player_pos() const {
	return { m_cvars.xp, m_cvars.yp };
}

sf::Vector2f world::get_player_vel() const {
	return { m_cvars.xv, m_cvars.yv };
}

sf::Vector2f world::get_player_scale() const {
	return m_player.getScale();
}

sf::Vector2i world::get_player_size() const {
	return m_player.size();
}

std::string world::get_player_anim() const {
	return m_player.get_animation();
}

input_state world::get_player_inputs() const {
	return m_cvars.this_frame;
}

bool world::get_player_grounded() const {
	return !m_first_input ? true : m_player_grounded();
}

world::control_vars world::get_player_control_vars() const {
	return m_cvars;
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
	m_cvars.xp			 = m_start_x + 0.499f;
	m_cvars.yp			 = m_start_y + 0.499f;
	m_cvars.xv			 = 0;
	m_cvars.yv			 = 0;
	m_cvars.flip_gravity = false;
	m_dead				 = false;
	m_first_input		 = false;
	m_sync_player_position();
	m_cvars.sx = 1;
	m_cvars.sy = 1;
	if (m_playback) {
		m_player.set_fill_color(m_playback->fill());
		m_player.set_outline_color(m_playback->outline());
	} else {
		m_player.set_fill_color(context::get().get_player_fill());
		m_player.set_outline_color(context::get().get_player_outline());
	}
	m_mt_mgr.restart();
	m_cvars.time_airborne  = sf::seconds(999);
	m_cvars.jumping		   = true;
	m_cvars.dashing		   = false;
	m_cvars.since_wallkick = sf::seconds(999);
	m_cvars.this_frame	   = input_state();
	m_cvars.last_frame	   = input_state();
	m_cvars.climbing	   = false;
	m_touched_goal		   = false;
	m_ctime				   = sf::Time::Zero;
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
	return has_playback() ? *m_playback : m_replay;
}

sf::Time world::get_timer() const {
	return sf::seconds(replay::timestep.asSeconds() * float(m_cstep));
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

bool world::m_alt_controls() const {
	if (has_playback())
		return m_playback->alt();
	else
		return context::get().alt_controls();
}

void world::run_controls(sf::Time dt, world::control_vars& v, particle_manager* pmgr) {
	// i copied the control code into this method to abstract it
	// to use in online interpolation
	const input_state this_frame = input_state::from_int(v.this_frame);
	const input_state last_frame = input_state::from_int(v.last_frame);
	const bool grounded			 = v.grounded;
	const dir facing			 = v.facing;
	const bool on_ice			 = v.on_ice;
	const auto against_ladder	 = [v](dir d) {
		   return d == dir::left ? v.against_ladder_left : v.against_ladder_right;
	};
	const auto can_player_wallkick = [v](dir d) {
		return d == dir::left ? v.can_wallkick_left : v.can_wallkick_right;
	};
	const bool flip_gravity = v.flip_gravity;
	const bool alt_controls = v.alt_controls;
	const bool tile_above	= v.tile_above;

	float gravity_sign = flip_gravity ? -1 : 1;

	// end redefinitions //

	if (this_frame.dash) {
		// can only start dashing if on the ground
		if (grounded && !v.climbing) {
			if (!v.dashing) {	// start of dash
				v.dash_dir = facing;
			}
			v.dashing = true;
		}
	} else if (grounded) {
		v.dashing = false;
	}

	float air_control_factor	  = grounded ? 1 : (v.dashing && this_frame.dash ? phys.dash_air_control : phys.air_control);
	float ground_control_factor	  = v.dashing && grounded ? 0 : 1;
	float wallkick_control_factor = v.is_wallkick_locked() ? 0 : 1;
	float friction_control_factor = on_ice && grounded ? phys.ice_friction : 1;
	if (v.dashing) {
		// if dashing, l/r controls are disabled and we accelerate at full speed in the dash direction
		float xv_sign = v.dash_dir == dir::left ? -1 : 1;
		if (grounded)
			v.xv += phys.dash_x_accel * dt.asSeconds() *
					air_control_factor *
					friction_control_factor *
					xv_sign;
		else
			v.dash_dir = facing;
	}
	// disables climbing if we're no longer against a ladder
	if (v.climbing && !against_ladder(v.climbing_facing)) {
		v.climbing = false;
		// if we're going "up"
		if (v.yv * gravity_sign < 0 && !v.is_wallkick_locked()) {
			v.xv = phys.climb_dismount_xv * (v.climbing_facing == dir::left ? -1 : 1);
		}
	}
	bool lr_inputted = false;
	if (this_frame.right) {
		lr_inputted = !lr_inputted;
		// wallkick
		if (can_player_wallkick(dir::left)) {
			v.player_wallkick(dir::left, pmgr);
		} else if (!v.climbing && against_ladder(dir::right)) {
			v.climbing		  = true;
			v.climbing_facing = dir::right;
			v.yv			  = 0;
		} else if (v.climbing && against_ladder(dir::left) && last_frame.right) {
			if (!alt_controls || grounded) v.climbing = false;
		} else if (v.climbing && alt_controls) {
			// no op
		} else if (!this_frame.left) {
			// normal acceleration
			if (v.xv < 0 && !on_ice) {
				v.xv += phys.x_decel * dt.asSeconds() *
						air_control_factor *
						ground_control_factor *
						wallkick_control_factor *
						friction_control_factor;
			}
			v.xv += phys.x_accel * dt.asSeconds() *
					air_control_factor *
					ground_control_factor *
					wallkick_control_factor *
					friction_control_factor;
			// we can only change direction when not dashing
			if ((!v.dashing || !grounded) && !v.is_wallkick_locked()) {
				v.sx = -1;
			}
		}
	}
	if (this_frame.left) {
		lr_inputted = !lr_inputted;
		// wallkick
		if (can_player_wallkick(dir::right)) {
			v.player_wallkick(dir::right, pmgr);
		} else if (!v.climbing && against_ladder(dir::left)) {
			v.climbing		  = true;
			v.climbing_facing = dir::left;
			v.yv			  = 0;
		} else if (v.climbing && against_ladder(dir::right) && last_frame.left) {
			if (!alt_controls || grounded) v.climbing = false;
		} else if (v.climbing && alt_controls) {
			// no op
		} else if (!this_frame.right) {
			// normal acceleration
			if (v.xv > 0 && !on_ice) {
				v.xv -= phys.x_decel * dt.asSeconds() *
						air_control_factor *
						ground_control_factor *
						wallkick_control_factor;
			}
			v.xv -= phys.x_accel * dt.asSeconds() *
					air_control_factor *
					ground_control_factor *
					wallkick_control_factor *
					friction_control_factor;
			// we can only change direction when not dashing
			if ((!v.dashing || !grounded) && !v.is_wallkick_locked())
				v.sx = 1;
		}
	}
	if (!lr_inputted && !v.dashing && !v.is_wallkick_locked()) {
		if (v.xv > (phys.x_decel / 2.f) * dt.asSeconds()) {
			v.xv -= phys.x_decel *
					friction_control_factor *
					dt.asSeconds();
		} else if (v.xv < (-phys.x_decel / 2.f) * dt.asSeconds()) {
			v.xv += phys.x_decel *
					friction_control_factor *
					dt.asSeconds();
		} else {
			v.xv = 0;
		}
	}

	if (this_frame.jump) {
		bool dismount_keyed = v.climbing && alt_controls && !this_frame.right && !this_frame.left && this_frame.jump;
		// normal jumping
		if (!v.jumping && !v.climbing && v.grounded_ago(sf::milliseconds(phys.coyote_millis)) && !tile_above) {
			v.yv = -phys.jump_v * gravity_sign;
			// so that we can't jump twice :)
			v.time_airborne = sf::seconds(999);
			v.jumping		= true;
			resource::get().play_sound("jump");
			// to prevent sticking
			v.yp -= 0.01f * gravity_sign;
		} else if (v.climbing && !last_frame.jump && dismount_keyed) {	 // if not jumping, we can dismount
			v.climbing = false;
		}

		// wallkicks
		if (can_player_wallkick(mirror(facing))) {
			v.player_wallkick(mirror(facing), pmgr);
		}
	} else {
		if (v.jumping) {
			v.jumping = false;
			if (!v.is_wallkick_locked())
				v.yv *= phys.shorthop_factor;
		}
	}

	if (v.climbing) {		   // up and down controls while climbing
		if (!alt_controls) {   // DEFAULT CONTROLS
			if (this_frame.up) {
				v.yv -= phys.climb_ya * dt.asSeconds() * gravity_sign;
				// to prevent sticking
				if (grounded)
					v.yp -= 0.01f * gravity_sign;
			} else if (this_frame.down) {
				v.yv += phys.climb_ya * dt.asSeconds() * gravity_sign;
			} else {
				if (v.yv > (phys.climb_ya / 2.f) * dt.asSeconds()) {
					v.yv -= phys.climb_ya * dt.asSeconds();
				} else if (v.yv < -(phys.climb_ya / 2.f) * dt.asSeconds()) {
					v.yv += phys.climb_ya * dt.asSeconds();
				} else {
					v.yv = 0;
				}
			}
			v.yv = util::clamp(v.yv, -phys.climb_yv_max, phys.climb_yv_max);
		} else {   // BLOCKBROS CONTROLS
			bool v_up_keyed	  = v.climbing_facing == dir::left ? this_frame.left : this_frame.right;
			bool v_down_keyed = v.climbing_facing == dir::left ? this_frame.right : this_frame.left;
			if (v_up_keyed) {
				v.yv -= phys.climb_ya * dt.asSeconds() * gravity_sign;
				// to prevent sticking
				if (grounded)
					v.yp -= 0.01f * gravity_sign;
			} else if (v_down_keyed) {
				v.yv += phys.climb_ya * dt.asSeconds() * gravity_sign;
			} else {
				if (v.yv > (phys.climb_ya / 2.f) * dt.asSeconds()) {
					v.yv -= phys.climb_ya * dt.asSeconds();
				} else if (v.yv < -(phys.climb_ya / 2.f) * dt.asSeconds()) {
					v.yv += phys.climb_ya * dt.asSeconds();
				} else {
					v.yv = 0;
				}
			}
			v.yv = util::clamp(v.yv, -phys.climb_yv_max, phys.climb_yv_max);
		}
	}
	/////////////////////

	float real_xv_max = v.dashing ? phys.dash_xv_max : phys.xv_max;
	v.xv			  = util::clamp(v.xv, -real_xv_max, real_xv_max);

	if (!v.climbing) {	 // no gravity while climbing
		v.yv += phys.grav * dt.asSeconds() * gravity_sign;
		if (flip_gravity) {
			v.yv = util::clamp(v.yv, -phys.yv_max, phys.jump_v);
		} else {
			v.yv = util::clamp(v.yv, -phys.jump_v, phys.yv_max);
		}
	}
}

void world::step(sf::Time dt) {
	// -1 if gravity is flipped. used for y velocity calculations

	m_cstep++;

	if (!m_playback && !m_dead) m_replay.push(m_cvars.this_frame);

	// update moving platforms
	m_mt_mgr.update(dt);

	// check if we're on a moving platform
	m_update_mp();
	// update "touching" list
	m_update_touching();

	// shift the player by the amount the platform they're on moved
	sf::Vector2f mp_offset = m_mp_player_offset(dt);
	m_cvars.xp += mp_offset.x;
	m_cvars.yp += mp_offset.y;

	bool grounded = m_player_grounded();
	if (grounded) {
		m_cvars.time_airborne = sf::seconds(0);
	} else {
		m_cvars.time_airborne += dt;
	}

	m_cvars.since_wallkick += dt;

	// controls //

	m_cvars.facing				 = m_facing();
	m_cvars.grounded			 = grounded;
	m_cvars.against_ladder_left	 = m_against_ladder(dir::left);
	m_cvars.against_ladder_right = m_against_ladder(dir::right);
	m_cvars.can_wallkick_left	 = m_can_player_wallkick(dir::left);
	m_cvars.can_wallkick_right	 = m_can_player_wallkick(dir::right);
	m_cvars.on_ice				 = m_on_ice();
	m_cvars.alt_controls		 = m_alt_controls();
	m_cvars.tile_above			 = m_tile_above_player();
	run_controls(dt, m_cvars);

	// !! physics !! //

	float initial_x	 = m_cvars.xp;
	float initial_y	 = m_cvars.yp;
	float intended_x = m_cvars.xp + m_cvars.xv * dt.asSeconds();
	float intended_y = m_cvars.yp + m_cvars.yv * dt.asSeconds();

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
									   ? pos.x + 1.5f - ((1 - player_size().x) / 2.f)	 // hitting right side of block
									   : pos.x - 0.5f + ((1 - player_size().x) / 2.f);	 // hitting left side of block
				x_collided		 = true;
				// edge case to solve a bug in which moving against a bottom-right corner would trap you
				if (cx < pos.x)
					intended_x -= m_cvars.xv < 0 ? 0.01f : 0;
				else if (cx > pos.x)
					intended_x += m_cvars.xv > 0 ? 0.01f : 0;
				// if we're walking into a moving platform moving away from us, then we want to follow it, not bounce off it
				std::optional<moving_tile> walking_into = m_moving_platform_handle[int(cx > pos.x ? dir::left : dir::right)];
				if (walking_into &&
					util::same_sign(walking_into->vel().x, m_cvars.xv) &&
					std::abs(m_cvars.xv) > 0.01f) {
					m_cvars.xv = walking_into->vel().x;
				} else {
					m_cvars.xv = 0;
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
									   : pos.y - 0.5f + ((1 - player_size().y) / 6.0f);	  // hitting top side of block
				cy				 = intended_y;
				y_collided		 = true;
				m_cvars.yv		 = 0;
			}
		}
	}


	m_cvars.xp = intended_x;
	m_cvars.yp = intended_y;

	// test for being squeezed
	if (m_player_is_squeezed() || m_player_oob()) {
		m_player_die();
	}

	// handle gravity blocks
	if (m_test_touching_any(m_cvars.flip_gravity ? dir::up : dir::down, [](tile t) { return t == tile::gravity; })) {
		// gravity particles
		auto& gp = m_pmgr.spawn<particles::gravity>(m_cvars.flip_gravity);
		for (auto& tile : m_touching[m_cvars.flip_gravity ? dir::up : dir::down]) {
			if (tile == tile::gravity) {
				gp.setPosition(tile.x() + 0.5f, tile.y() + (m_cvars.flip_gravity ? 1 : 0));
				break;
			}
		}

		resource::get().play_sound("gravityflip");
		m_cvars.flip_gravity = !m_cvars.flip_gravity;
		m_cvars.dashing		 = false;
		m_cvars.yv			 = m_cvars.flip_gravity ? -0.1f : 0.1f;
		m_cvars.sy			 = m_cvars.flip_gravity ? -1 : 1;
	}

	// update last frame keys
	m_cvars.last_frame = m_cvars.this_frame;
}

void world::m_check_colors() {
	if (has_playback()) return;
	if (m_player.get_fill_color() != context::get().get_player_fill()) {
		m_player.set_fill_color(context::get().get_player_fill());
	}
	if (m_player.get_outline_color() != context::get().get_player_outline()) {
		m_player.set_outline_color(context::get().get_player_outline());
	}
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
		m_first_input	   = true;
		m_cvars.this_frame = m_playback->get(m_cstep);
	} else {
		m_cvars.this_frame.left	 = m_has_focus ? settings::get().key_down(key::LEFT) : false;
		m_cvars.this_frame.right = m_has_focus ? settings::get().key_down(key::RIGHT) : false;
		m_cvars.this_frame.dash	 = m_has_focus ? settings::get().key_down(key::DASH) : false;
		m_cvars.this_frame.jump	 = m_has_focus ? settings::get().key_down(key::JUMP) : false;
		m_cvars.this_frame.up	 = m_has_focus ? settings::get().key_down(key::UP) : false;
		m_cvars.this_frame.down	 = m_has_focus ? settings::get().key_down(key::DOWN) : false;
	}

	if (settings::get().key_down(key::RESTART) && !ImGui::GetIO().WantCaptureKeyboard && resource::get().window().hasFocus()) {
		if (!m_restarted)
			m_restart_world();
		m_restarted = true;
	} else {
		m_restarted = false;
	}

	m_pmgr.update(dt);

	// update the player's animations
	m_check_colors();
	m_player.update();

	if (won()) {
		sf::Color opacity = m_space_to_retry.getColor();
		m_end_alpha += 255.f * dt.asSeconds();
		m_end_alpha = std::min(255.f, m_end_alpha);
		opacity.a	= m_end_alpha;
		m_space_to_retry.setColor(opacity);
		m_game_clear.setColor(opacity);
		m_fadeout.setFillColor(sf::Color(220, 220, 220, m_end_alpha / 2.f));
		if (m_cvars.last_frame.jump && !m_cvars.this_frame.jump && !ImGui::GetIO().WantCaptureKeyboard) {
			m_restart_world();
			return false;
		} else {
			m_cvars.last_frame.jump = m_cvars.this_frame.jump;
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
		if (m_cvars.last_frame.jump && !m_cvars.this_frame.jump) {
			m_restart_world();
			return false;
		} else {
			m_cvars.last_frame.jump = m_cvars.this_frame.jump;
			return false;
		}
	}

	m_first_input |= m_cvars.this_frame.left || m_cvars.this_frame.right || m_cvars.this_frame.jump || m_cvars.this_frame.dash || m_cvars.this_frame.down || m_cvars.this_frame.up;

	// physics updates!
	bool stepped = false;
	if (m_first_input) {
		m_ctime += dt;
		while (m_ctime > replay::timestep) {
			m_ctime -= replay::timestep;
			step(replay::timestep);
			stepped = true;
		}
	} else {
		stepped = true;
	}

	// debug::get().box(util::scale<float>(m_get_player_top_aabb(m_cvars.xp, m_cvars.yp), m_player.size().x));
	// debug::get().box(util::scale<float>(m_get_player_bottom_aabb(m_cvars.xp, m_cvars.yp), m_player.size().x));
	//  debug::get().box(util::scale<float>(m_get_player_left_aabb(m_cvars.xp, m_cvars.yp), m_player.size().x));
	//  debug::get().box(util::scale<float>(m_get_player_right_aabb(m_cvars.xp, m_cvars.yp), m_player.size().x));
	//  debug::get().box(util::scale<float>(m_get_player_top_ghost_aabb(m_cvars.xp, m_cvars.yp), m_player.size().x));
	//  debug::get().box(util::scale<float>(m_get_player_bottom_ghost_aabb(m_cvars.xp, m_cvars.yp), m_player.size().x));
	//  debug::get().box(util::scale<float>(m_get_player_left_ghost_aabb(m_cvars.xp, m_cvars.yp), m_player.size().x));
	//  debug::get().box(util::scale<float>(m_get_player_right_ghost_aabb(m_cvars.xp, m_cvars.yp), m_player.size().x));
	//  debug::get().box(util::scale<float>(m_get_player_aabb(m_cvars.xp, m_cvars.yp), m_player.size().x));
	//  debug::get().box(util::scale<float>(m_get_player_x_aabb(m_cvars.xp, m_cvars.yp), m_player.size().x));
	// debug::get().box(util::scale<float>(m_get_player_y_ghost_aabb(m_cvars.xp, m_cvars.yp), m_player.size().x));


	// debug::get() << "dashing = " << m_cvars.dashing << "\n";
	// debug::get() << "airborne = " << m_cvars.time_airborne.asSeconds() << "\n";
	// debug::get() << "climbing = " << m_cvars.climbing << "\n";
	// debug::get() << "won = " << won() << "\n";

	// some debug info
	// debug::get() << "dt = " << dt.asMilliseconds() << "ms\n";
	// debug::get() << "velocity = " << sf::Vector2f(m_cvars.xv, m_cvars.yv) << "\n";
	debug::get() << "touching Y-: " << m_touching[int(dir::up)] << "\n";
	debug::get() << "touching Y+: " << m_touching[int(dir::down)] << "\n";
	debug::get() << "touching X-: " << m_touching[int(dir::left)] << "\n";
	debug::get() << "touching X+: " << m_touching[int(dir::right)] << "\n";

	// debug::get() << "mp u="
	//<< !!m_moving_platform_handle[0]
	//<< " d="
	//<< !!m_moving_platform_handle[2]
	//<< " r="
	//<< !!m_moving_platform_handle[1]
	//<< " l="
	//<< !!m_moving_platform_handle[3]
	//<< "\n";

	// update the player's animation
	m_update_animation();
	m_sync_player_position();

	return stepped;
}

void world::control_vars::player_wallkick(dir d, particle_manager* pmgr) {
	if (is_wallkick_locked()) return;
	float xv_sign	= d == dir::left ? -1 : 1;
	float grav_sign = flip_gravity ? -1 : 1;
	climbing		= false;
	xv				= phys.wallkick_xv * xv_sign;
	yv				= -phys.wallkick_yv * grav_sign;
	xp += ((1 - player_size().x) / 2.f) * xv_sign;
	if (pmgr) {
		auto& sp = pmgr->spawn<particles::smoke>();
		sp.setPosition(xp - 0.35f * xv_sign, yp);
		sp.setScale(xv_sign, sp.getScale().y);
	}
	resource::get().play_sound("wallkick");
	sx			   = -xv_sign;
	since_wallkick = sf::Time::Zero;
}

bool world::control_vars::is_wallkick_locked() const {
	return since_wallkick < sf::milliseconds(200);
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
		sf::FloatRect aabb = m_get_player_ghost_aabb(m_cvars.xp, m_cvars.yp, dir(i));
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
		sf::FloatRect aabb = m_get_player_ghost_aabb(m_cvars.xp, m_cvars.yp, dir(i));
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
	std::optional<moving_tile> standing_on = m_moving_platform_handle[int(m_cvars.flip_gravity ? dir::up : dir::down)];
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
	if (left && tile(*left) == tile::ladder && m_cvars.climbing && m_cvars.climbing_facing == dir::left) {
		offset.y += left->vel().y * dt.asSeconds();
	}
	if (right && tile(*right) == tile::ladder && m_cvars.climbing && m_cvars.climbing_facing == dir::right) {
		offset.y += right->vel().y * dt.asSeconds();
	}

	// specific case for climbing on a moving ladder going away from us
	if (m_cvars.climbing) {
		if (m_cvars.climbing_facing == dir::left && left && left->vel().x < 0) {
			offset.x += left->vel().x * dt.asSeconds();
		}
		if (m_cvars.climbing_facing == dir::right && right && right->vel().x > 0) {
			offset.x += right->vel().x * dt.asSeconds();
		}
	}

	return offset;
}

void world::m_update_animation() {
	if (m_cvars.climbing) {
		if (std::abs(m_cvars.yv) > 0.01f) {
			m_player.set_animation("climb");
		} else {
			m_player.set_animation("hang");
		}
		m_cvars.sx = m_cvars.climbing_facing == dir::left ? 1 : -1;
	} else if (std::abs(m_cvars.xv) > phys.xv_max) {
		m_player.set_animation("dash");
	} else if (std::abs(m_cvars.xv) > 0.3f) {
		if (m_on_ice()) {
			if (m_cvars.this_frame.left || m_cvars.this_frame.right || m_cvars.this_frame.dash) {
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
	if (!m_cvars.climbing && std::abs(m_cvars.yv) > 0) {
		if (m_cvars.flip_gravity) {
			if (m_cvars.yv > -phys.yv_max * 0.2f) {
				m_player.set_animation("jump");
			} else if (m_cvars.yv < -phys.yv_max * 0.5f) {
				m_player.set_animation("fall");
			}
		} else {
			if (m_cvars.yv < phys.yv_max * 0.2f) {
				m_player.set_animation("jump");
			} else if (m_cvars.yv > phys.yv_max * 0.5f) {
				m_player.set_animation("fall");
			}
		}
	}

	m_player.setScale(m_cvars.sx, m_cvars.sy);
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
	m_touched_goal	= true;
	m_end_alpha		= 0;
	m_cvars.dashing = false;
	m_space_to_retry.setColor(sf::Color(255, 255, 255, 0));
	m_game_over.setColor(sf::Color(255, 255, 255, 0));
	m_game_clear.setColor(sf::Color(255, 255, 255, 0));
	resource::get().play_sound("victory");
	auto& sp		= m_pmgr.spawn<particles::victory>();
	float grav_sign = m_cvars.flip_gravity ? -1 : 1;
	sp.setPosition(m_cvars.xp, m_cvars.yp + 0.2f * grav_sign);
}

void world::m_player_die() {
	debug::log() << "death report:\n";
	debug::log() << "velocity = " << sf::Vector2f(m_cvars.xv, m_cvars.yv) << "\n";
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
	m_end_alpha		= 0;
	m_cvars.dashing = false;
	m_space_to_retry.setColor(sf::Color(255, 255, 255, 0));
	m_game_over.setColor(sf::Color(255, 255, 255, 0));
	m_game_clear.setColor(sf::Color(255, 255, 255, 0));
	auto& dp = m_pmgr.spawn<particles::death>();
	dp.setPosition(m_cvars.xp, m_cvars.yp);
	dp.setScale(0.5f, 0.5f);
}

void world::m_sync_player_position() {
	float xp = m_cvars.xp + m_cvars.xv * dt_since_step();
	float yp = m_cvars.yp + m_cvars.yv * dt_since_step();
	m_player.setPosition(xp * m_player.size().x, yp * m_player.size().y);
}

bool world::m_on_ice() const {
	return m_test_touching_any(m_cvars.flip_gravity ? dir::up : dir::down, [](tile t) -> bool {
		return t == tile::ice;
	});
}

bool world::m_player_grounded() const {
	return m_test_touching_any(m_cvars.flip_gravity ? dir::up : dir::down, [](tile t) { return t.solid(); });
}

bool world::control_vars::grounded_ago(sf::Time t) const {
	return time_airborne < t;
}

bool world::m_just_jumped() const {
	return !m_cvars.last_frame.jump && m_cvars.this_frame.jump;
}

bool world::m_player_oob() const {
	bool fell_through_bottom = !m_cvars.flip_gravity ? m_cvars.yp > m_level.map().size().y : m_cvars.yp < -1;
	return m_cvars.xp < -1 || m_cvars.xp > m_level.map().size().x || fell_through_bottom;
}

bool world::m_against_ladder(dir d) const {
	return m_test_touching_any(d, [](tile t) {
		return t == tile::ladder;
	});
}

bool world::m_can_player_wallkick(dir d, bool keys_pressed) const {
	bool keyed_last_frame = d == dir::left ? m_cvars.last_frame.left : m_cvars.last_frame.right;
	bool keyed_this_frame = d == dir::left ? m_cvars.this_frame.left : m_cvars.this_frame.right;
	bool jump_this_frame  = !m_cvars.last_frame.jump && m_cvars.this_frame.jump;
	bool just_keyed		  = !keyed_last_frame && keyed_this_frame;

	bool key_condition = !keys_pressed || just_keyed || jump_this_frame;

	bool alt_no_ladder_jump_cond = true;
	if (m_alt_controls()) {	  // disable walljumping against ladders with l/r in the alt control scheme
		alt_no_ladder_jump_cond = !m_against_ladder(d == dir::left ? dir::right : dir::left) || jump_this_frame;
	}

	return key_condition && alt_no_ladder_jump_cond && !m_player_grounded() &&
		   m_test_touching_any(d == dir::left ? dir::right : dir::left, [](tile t) {
			   return t.solid() && !t.blocks_wallkicks();
		   });
}

bool world::m_tile_above_player() const {
	return m_test_touching_any(m_cvars.flip_gravity ? dir::down : dir::up, [](tile t) { return t.solid(); });
}

world::dir world::m_facing() const {
	return m_player.getScale().x < 0 ? dir::right : dir::left;
}

sf::Vector2f world::player_size() {
	return { 0.6f, 0.7f };
}

sf::FloatRect world::m_get_player_aabb(float x, float y) const {
	// aabb will be the sprite's full hitbox, minus 3 px on the x axis
	sf::FloatRect ret;
	ret.left   = x - 0.5f + ((1 - player_size().x) / 2.f);
	ret.top	   = y - 0.5f + ((1 - player_size().y) / 1.2f);
	ret.width  = player_size().x;
	ret.height = player_size().y;

	if (m_cvars.flip_gravity) {
		ret.top -= ((1 - player_size().y) / 1.2f);
		ret.height += ((1 - player_size().y) / 1.2f);
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

constexpr world::dir world::mirror(dir d) {
	switch (d) {
	case dir::up: return dir::down;
	case dir::down: return dir::up;
	case dir::left: return dir::right;
	case dir::right: return dir::left;
	};
}
