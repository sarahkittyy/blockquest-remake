#pragma once

#include "../particles.hpp"

namespace particles {

class gravity : public particle_system {
public:
	gravity(bool upside_down);
	~gravity();
};

}
