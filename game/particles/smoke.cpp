#include "smoke.hpp"
#include <SFML/System/Time.hpp>

namespace particles {

smoke::smoke(resource& r)
	: particle_system(r.tex("assets/particles/smoke.png")) {
	int dur_ms = 200;
	emit({
		.lifetime = sf::milliseconds(dur_ms),
		.tx		  = 3,
		.ty		  = 3,
		.tc		  = 8,
		.fs		  = sf::milliseconds(dur_ms / 8),
	});
}

smoke::~smoke() {
}

}
