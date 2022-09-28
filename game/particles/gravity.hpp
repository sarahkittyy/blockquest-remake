#pragma once

#include "../particles.hpp"

namespace particles {

class gravity : public particle_system {
public:
	gravity(resource& r, bool upside_down);
	~gravity();
};

}
