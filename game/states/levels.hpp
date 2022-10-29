#pragma once

#include "../fsm.hpp"

namespace states {

/* locally saved levels display */
class levels : public state {
public:
	levels();
	~levels();

	void update(fsm* sm, sf::Time dt);
};

}
