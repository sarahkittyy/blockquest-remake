#include "gravity.hpp"

#include "resource.hpp"

namespace particles {

gravity::gravity(bool upside_down)
	: particle_system(resource::get().tex("assets/particles/gravity.png")) {
	float sign = upside_down ? -1 : 1;
	emit({
		.lifetime = sf::milliseconds(300),
		.yv		  = -2.f * sign,
		.ya		  = 2.f * sign,
		.alpha_v  = -2.f,
		.xs		  = 0.5f,
		.xs_v	  = -0.5f,
		.ys		  = 0.5f,
	});
	emit({
			 .lifetime = sf::milliseconds(300),
			 .yv	   = -2.f * sign,
			 .ya	   = 2.f * sign,
			 .alpha_v  = -2.f,
			 .xs	   = 0.65f,
			 .xs_v	   = -0.65f,
			 .ys	   = 0.5f,
		 },
		 sf::milliseconds(100));
	emit({
			 .lifetime = sf::milliseconds(300),
			 .yv	   = -2.f * sign,
			 .ya	   = 2.f * sign,
			 .alpha_v  = -2.f,
			 .xs	   = 0.8f,
			 .xs_v	   = -0.8f,
			 .ys	   = 0.5f,
		 },
		 sf::milliseconds(200));
}

gravity::~gravity() {
}

}
