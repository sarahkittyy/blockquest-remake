#pragma once

#include "multiplayer.hpp"
#include "player.hpp"

class player_ghost : public sf::Drawable, public sf::Transformable {
public:
	player_ghost();

	void update();

	void flush_state(const multiplayer::player_state& s);
	void flush_data(const multiplayer::player_data& d);

private:
	void draw(sf::RenderTarget& t, sf::RenderStates s) const;

	uint64_t m_last_update;
	player m_p;
	multiplayer::player_state m_state;
	multiplayer::player_data m_data;
};
