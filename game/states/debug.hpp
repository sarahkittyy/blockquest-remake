#pragma once

#include "../fsm.hpp"
#include "../level.hpp"
#include "../world.hpp"

#include <memory>

namespace states {

/* for testing physics */
class debug : public state {
public:
	debug();
	~debug();

	void update(fsm* sm, sf::Time dt);

private:
	void draw(sf::RenderTarget&, sf::RenderStates) const;

	std::unique_ptr<world> m_w;
};

}
