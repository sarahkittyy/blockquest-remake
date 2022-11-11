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

char* base64_encode(char* plain) {
	char counts = 0;
	char buffer[3];
	char* cipher = new char[strlen(plain) * 4 / 3 + 4 + 1]{ 0 };
	int i = 0, c = 0;

	for (i = 0; plain[i] != '\0'; i++) {
		buffer[counts++] = plain[i];
		if (counts == 3) {
			cipher[c++] = base46_map[buffer[0] >> 2];
			cipher[c++] = base46_map[((buffer[0] & 0x03) << 4) + (buffer[1] >> 4)];
			cipher[c++] = base46_map[((buffer[1] & 0x0f) << 2) + (buffer[2] >> 6)];
			cipher[c++] = base46_map[buffer[2] & 0x3f];
			counts		= 0;
		}
	}

	if (counts > 0) {
		cipher[c++] = base46_map[buffer[0] >> 2];
		if (counts == 1) {
			cipher[c++] = base46_map[(buffer[0] & 0x03) << 4];
			cipher[c++] = '=';
		} else {   // if counts == 2
			cipher[c++] = base46_map[((buffer[0] & 0x03) << 4) + (buffer[1] >> 4)];
			cipher[c++] = base46_map[(buffer[1] & 0x0f) << 2];
		}
		cipher[c++] = '=';
	}

	cipher[c] = '\0'; /* string padding character */
	return cipher;
}


char* base64_decode(char* cipher) {
	char counts = 0;
	char buffer[4];
	char* plain = new char[strlen(cipher) * 3 / 4 + 1]{ 0 };
	int i = 0, p = 0;

	for (i = 0; cipher[i] != '\0'; i++) {
		char k;
		for (k = 0; k < 64 && base46_map[k] != cipher[i]; k++)
			;
		buffer[counts++] = k;
		if (counts == 4) {
			plain[p++] = (buffer[0] << 2) + (buffer[1] >> 4);
			if (buffer[2] != 64)
				plain[p++] = (buffer[1] << 4) + (buffer[2] >> 2);
			if (buffer[3] != 64)
				plain[p++] = (buffer[2] << 6) + buffer[3];
			counts = 0;
		}
	}

	plain[p] = '\0'; /* string padding character */
	return plain;
}

}
