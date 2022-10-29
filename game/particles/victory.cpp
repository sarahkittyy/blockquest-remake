#include "victory.hpp"

#include "../util.hpp"

#include "../resource.hpp"

namespace particles {

victory::victory()
	: particle_system(resource::get().tex("assets/particles/death.png")) {
	const int ct	= 30;
	const int dur	= 1000;
	const int delay = dur / ct;
	for (int i = 0; i < ct; i++) {
		const float xv_min = -3.f;
		const float xv_max = 3.f;
		const float yv_min = -8.f;
		const float yv_max = -3.f;
		float xrand		   = rand() / float(RAND_MAX);
		float yrand		   = rand() / float(RAND_MAX);
		const float xv	   = util::lerp(xv_min, xv_max, xrand);
		const float yv	   = util::lerp(yv_min, yv_max, yrand);
		emit({
				 .lifetime = sf::milliseconds(dur),
				 .xp	   = 0,
				 .yp	   = 0,
				 .xv	   = xv,
				 .yv	   = yv,
				 .ya	   = 10.f,
				 .alpha_v  = (1000.f / dur) * -0.8f,
			 },
			 sf::milliseconds(delay * i));
	}
}

victory::~victory() {
}

}
