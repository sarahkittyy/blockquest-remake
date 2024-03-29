#include "util.hpp"

#include <SFML/Graphics.hpp>
#include <chrono>
#include <cmath>

#include "tilemap.hpp"

namespace util {

uint64_t get_time() {
	return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

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

static char base46_map[] = { 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
							 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
							 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
							 'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/' };
static std::string cvt =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZ"

	// 22223333333333444444444455
	// 67890123456789012345678901
	"abcdefghijklmnopqrstuvwxyz"

	// 555555556666
	// 234567890123
	"0123456789+/";

std::string base64_encode(const std::vector<char>& data) {
	std::string ret;
	int val	 = 0;
	int valb = -6;
	for (unsigned char c : data) {
		val = (val << 8) + c;
		valb += 8;
		while (valb >= 0) {
			ret.push_back(cvt[(val >> valb) & 0x3F]);
			valb -= 6;
		}
	}
	if (valb > -6) ret.push_back(cvt[((val << 8) >> (valb + 8)) & 0x3F]);
	while (ret.size() % 4) ret.push_back('=');
	return ret;
}

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

}
