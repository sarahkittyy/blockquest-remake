#pragma once

#include "../particles.hpp"

namespace particles {

class smoke : public particle_system {
public:
	smoke(resource& r);
	~smoke();
};

}
