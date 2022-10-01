#pragma once

#include "../fsm.hpp"
#include "../level.hpp"
#include "../world.hpp"

#include <memory>

namespace states {

class debug : public state {
public:
	debug(resource& r);
	~debug();

	void update(fsm* sm, sf::Time dt);

private:
	void draw(sf::RenderTarget&, sf::RenderStates) const;

	std::unique_ptr<world> m_w;
};

}
