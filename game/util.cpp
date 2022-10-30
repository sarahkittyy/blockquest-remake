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

struct BASE64_DEC_TABLE {
	signed char n[256];

	BASE64_DEC_TABLE() {
		for (int i = 0; i < 256; ++i) n[i] = -1;
		for (unsigned char i = '0'; i <= '9'; ++i) n[i] = 52 + i - '0';
		for (unsigned char i = 'A'; i <= 'Z'; ++i) n[i] = i - 'A';
		for (unsigned char i = 'a'; i <= 'z'; ++i) n[i] = 26 + i - 'a';
		n['+'] = 62;
		n['/'] = 63;
	}
	int operator[](unsigned char i) const {
		return n[i];
	}
};
size_t base64_decode(const std::string& source, void* pdest, size_t dest_size) {
	static const BASE64_DEC_TABLE b64table;
	if (!dest_size) return 0;
	const size_t len = source.length();
	int bc = 0, a = 0;
	char* const pstart = static_cast<char*>(pdest);
	char* pd		   = pstart;
	char* const pend   = pd + dest_size;
	for (size_t i = 0; i < len; ++i) {
		const int n = b64table[source[i]];
		if (n == -1) continue;
		a |= (n & 63) << (18 - bc);
		if ((bc += 6) > 18) {
			*pd = a >> 16;
			if (++pd >= pend) return pd - pstart;
			*pd = a >> 8;
			if (++pd >= pend) return pd - pstart;
			*pd = a;
			if (++pd >= pend) return pd - pstart;
			bc = a = 0;
		}
	}
	if (bc >= 8) {
		*pd = a >> 16;
		if (++pd >= pend) return pd - pstart;
		if (bc >= 16) *(pd++) = a >> 8;
	}
	return pd - pstart;
}

bool same_sign(float a, float b) {
	return a * b >= 0.0f;
}

bool neither_zero(float a, float b) {
	return std::abs(a) > 0.001f && std::abs(b) > 0.001f;
}

float deg2rad(float d) {
	return (d * M_PI) / 180.f;
}

float rad2deg(float r) {
	return (r * 180.f) / M_PI;
}

}
