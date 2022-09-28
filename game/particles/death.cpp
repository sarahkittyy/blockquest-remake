#include "death.hpp"

#include <cmath>
#include "../util.hpp"

namespace particles {

death::death(resource& r)
	: particle_system(r.tex("assets/particles/death.png")) {
	float v = 2.5f;
	int dur = 1000;
	for (int i = 0; i < 360; i += 45) {
		float xv = v * std::cos(util::deg2rad(i));
		float yv = v * std::sin(util::deg2rad(i));
		emit({
			.xp		  = 0,
			.yp		  = 0,
			.xv		  = xv,
			.yv		  = yv,
			.lifetime = sf::milliseconds(dur),
			.alpha_v  = (1000.f / dur) * -1.f,
		});
	}
}

death::~death() {
}

}
