#pragma once

#include "../fsm.hpp"

namespace states {

class main : public state {
public:
	main(resource& r);
	~main();

	void update(fsm* sm, sf::Time dt);

	void imdraw(fsm* sm);

private:
	void draw(sf::RenderTarget&, sf::RenderStates) const;

	// some textures
	sf::Texture& m_logo_new;
	sf::Texture& m_play;
	sf::Texture& m_create;
};

}
