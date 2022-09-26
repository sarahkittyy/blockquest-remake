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

bool same_sign(float a, float b) {
	return a * b >= 0.0f;
}

}
