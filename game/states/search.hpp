#pragma once

#include "../fsm.hpp"

namespace states {

class search : public state {
public:
	search();
	~search();

	void update(fsm* sm, sf::Time dt);

	void imdraw(fsm* sm);

private:
	void draw(sf::RenderTarget&, sf::RenderStates) const;

	// some textures
	sf::Texture& m_logo_new;
	sf::Texture& m_play;
};

}
