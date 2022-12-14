#include "smoke.hpp"
#include <SFML/System/Time.hpp>

#include "../resource.hpp"

namespace particles {

smoke::smoke()
	: particle_system(resource::get().tex("assets/particles/smoke.png")) {
	int dur_ms = 250;
	emit({
		.lifetime = sf::milliseconds(dur_ms),
		.yp		  = -0.1f,
		.xs		  = 1.25f,
		.ys		  = 1.25f,
		.tx		  = 3,
		.ty		  = 3,
		.tc		  = 8,
		.fs		  = sf::milliseconds(dur_ms / 8),
	});
}

smoke::~smoke() {
}

}
