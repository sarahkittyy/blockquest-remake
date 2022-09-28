#pragma once

#include "../particles.hpp"

namespace particles {

class death : public particle_system {
public:
	death(resource& r);
	~death();
};

}
