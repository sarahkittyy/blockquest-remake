#pragma once

#include "multiplayer.hpp"
#include "nametag.hpp"
#include "player.hpp"

class player_ghost : public sf::Drawable, public sf::Transformable {
public:
	player_ghost();

	void update();

	void flush_state(const multiplayer::player_state& s);
	void flush_data(const multiplayer::player_data& d);

	void set_particle_manager(particle_manager* pmgr = nullptr);

private:
	void draw(sf::RenderTarget& t, sf::RenderStates s) const;

	uint64_t m_last_update;
	player m_p;
	nametag m_nametag;
	multiplayer::player_state m_state;
	multiplayer::player_data m_data;

	particle_manager* m_pmgr;
};
