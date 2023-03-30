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
	m_p.update();

	uint64_t ct	  = util::get_time();
	sf::Time dt	  = sf::milliseconds(ct - m_last_update);
	m_last_update = ct;

	debug::get() << "t: " << dt.asSeconds() << "\n";

	// mini physics simulation
	world::control_vars& v = m_state.controls;
	world::run_controls(dt, v);

	v.xp += v.xv * dt.asSeconds();
	if (v.grounded) {
		v.yv = 0;
	}
	v.yp += v.yv * dt.asSeconds();

	m_p.setPosition(v.xp * m_p.size().x, v.yp * m_p.size().y);
	m_nametag.setPosition(m_p.getPosition().x, m_p.getPosition().y - m_p.size().y);
	m_p.setScale(v.sx, v.sy);
}

void player_ghost::draw(sf::RenderTarget& t, sf::RenderStates s) const {
	if (m_state.controls.xp == -999) return;
	s.transform *= getTransform();
	// for the extra border tile offset
	s.transform.translate(1 * m_p.size().x, 0);
	t.draw(m_p, s);
	t.draw(m_nametag, s);
}
