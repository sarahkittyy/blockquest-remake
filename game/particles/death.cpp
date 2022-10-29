#include "death.hpp"

#include <cmath>
#include "../util.hpp"

#include "resource.hpp"

namespace particles {

death::death()
	: particle_system(resource::get().tex("assets/particles/death.png")) {
	float v		= 5.f;
	int dur		= 500;
	float loops = 8;
	for (int i = 0; i < 360 * loops; i += 137.508f) {
		float xv = v * std::cos(util::deg2rad(i));
		float yv = v * std::sin(util::deg2rad(i));
		emit({
				 .lifetime = sf::milliseconds(dur),
				 .xp	   = 0,
				 .yp	   = 0,
				 .xv	   = xv,
				 .yv	   = yv,
				 .alpha_v  = (1000.f / dur) * -0.8f,
			 },
			 sf::milliseconds((i / (360.f * loops)) * 500));
	}
}

death::~death() {
}

}
