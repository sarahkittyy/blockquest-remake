#include "util.hpp"

#include <SFML/Graphics.hpp>
#include <cmath>

#include "tilemap.hpp"

namespace util {

float clamp(float v, float min, float max) {
	return std::max(std::min(v, max), min);
}

float lerp(float min, float max, float t) {
	return ((max - min) * t) + min;
}

std::vector<std::pair<sf::Vector2f, tile>>
merge_contacts(const std::vector<std::pair<sf::Vector2f, tile>>& a, const std::vector<std::pair<sf::Vector2f, tile>>& b) {
	std::vector<std::pair<sf::Vector2f, tile>> ret(a);
	ret.insert(ret.end(), b.cbegin(), b.cend());
	return ret;
}

sf::Vector2f normalize(sf::Vector2f in) {
	float mag = std::hypot(in.x, in.y);
	in.x /= mag;
	in.y /= mag;
	return in;
}

bool same_sign(float a, float b) {
	return a * b >= 0.0f;
}

bool neither_zero(float a, float b) {
	return a != 0 && b != 0;
}

float deg2rad(float d) {
	return (d * M_PI) / 180.f;
}

float rad2deg(float r) {
	return (r * 180.f) / M_PI;
}

}
