#include "player_ghost.hpp"

#include "debug.hpp"
#include "replay.hpp"
#include "util.hpp"
#include "world.hpp"

player_ghost::player_ghost() {
	m_last_update = util::get_time();
	m_p.setOrigin(m_p.size().x / 2.f, m_p.size().y / 2.f);
	m_p.update();
}

void player_ghost::flush_state(const multiplayer::player_state& s) {
	m_state		  = s;
	m_last_update = m_state.updatedAt;
}

void player_ghost::flush_data(const multiplayer::player_data& d) {
	m_data = d;
	m_nametag.set_name(m_data.name);
}

void player_ghost::update() {
	if (m_p.get_fill_color() != m_data.fill) {
		m_p.set_fill_color(m_data.fill);
	}
	if (m_p.get_outline_color() != m_data.outline) {
		m_p.set_outline_color(m_data.outline);
	}
	if (m_p.get_animation() != m_state.anim) {
		m_p.set_animation(m_state.anim);
	}
	if (m_p.getScale().x != m_state.sx || m_p.getScale().y != m_state.sy) {
		m_p.setScale(m_state.sx, m_state.sy);
	}
	m_p.update();

	input_state input = input_state::from_int(m_state.inputs);
	auto phys		  = world::phys;
	uint64_t ct		  = util::get_time();
	float t			  = (ct - m_last_update) / 1000.f;
	m_last_update	  = ct;

	debug::get() << "t: " << t << "\n";

	// mini physics simulation
	bool lr_inputted = false;
	if (input.right) {
		lr_inputted = !lr_inputted;
		// wallkick
		if (!input.left) {
			// normal acceleration
			if (m_state.xv < 0) {
				m_state.xv += phys.x_decel * t;
			}
			m_state.xv += phys.x_accel * t;
		}
	}
	if (input.left) {
		lr_inputted = !lr_inputted;
		if (!input.right) {
			// normal acceleration
			if (m_state.xv > 0) {
				m_state.xv -= phys.x_decel * t;
			}
			m_state.xv -= phys.x_accel * t;
		}
	}
	if (!lr_inputted) {
		if (m_state.xv > (phys.x_decel / 2.f) * t) {
			m_state.xv -= phys.x_decel * t;
		} else if (m_state.xv < (-phys.x_decel / 2.f) * t) {
			m_state.xv += phys.x_decel * t;
		} else {
			m_state.xv = 0;
		}
	}

	m_state.xv = util::clamp(m_state.xv, -phys.xv_max, phys.xv_max);   // cap xv

	// float gravity_sign = m_state.sy < 0 ? -1.f : 1.f;

	// if (!m_state.grounded) { m_state.yv += phys.grav * t * gravity_sign; }	 // gravity
	// if (gravity_sign < 0) {													 // cap yv
	// m_state.yv = util::clamp(m_state.yv, -phys.yv_max, phys.jump_v);
	//} else {
	// m_state.yv = util::clamp(m_state.yv, -phys.jump_v, phys.yv_max);
	//}

	m_state.xp += m_state.xv * t;
	// m_state.yp += m_state.yv * t;
	m_p.setPosition(m_state.xp * m_p.size().x, m_state.yp * m_p.size().y);
	m_nametag.setPosition(m_p.getPosition().x, m_p.getPosition().y - m_p.size().y);
}

void player_ghost::draw(sf::RenderTarget& t, sf::RenderStates s) const {
	if (m_state.xp == -999) return;
	s.transform *= getTransform();
	// for the extra border tile offset
	s.transform.translate(1 * m_p.size().x, 0);
	t.draw(m_p, s);
	t.draw(m_nametag, s);
}
