#pragma once

#include <SFML/Graphics.hpp>
#include <chrono>
#include <future>
#include <string>
#include <vector>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

struct tile;

// various utility functions namespaced for organization
namespace util {

float clamp(float v, float min, float max);	  // keeps a value between two values

/**
 * @brief returns a value between min and max based on 0.0 <= t <= 1.0
 *
 * @param min the minimum value it can take on (t = 0)
 * @param max the maximum value it can take on (t = 1)
 * @param t the interpolation factor
 * @return the interpolated value
 */
float lerp(float min, float max, float t);

// returns true if the two numbers are the same sign
bool same_sign(float a, float b);

// returns true if neither number is zero
bool neither_zero(float a, float b);

/**
 * @brief merge two contact lists together
 *
 * @param a
 * @param b
 */
std::vector<std::pair<sf::Vector2f, tile>>
merge_contacts(const std::vector<std::pair<sf::Vector2f, tile>>& a, const std::vector<std::pair<sf::Vector2f, tile>>& b);

template <typename T>
sf::Rect<T> scale(sf::Rect<T> r, T f) {	  // scale a rectangle by a scalar
	return sf::Rect<T>(r.left * f, r.top * f, r.width * f, r.height * f);
}

sf::Vector2f normalize(sf::Vector2f in);

// for hashing sf::Vector2<T> instances
template <class T>
struct vector_hash {
	std::size_t operator()(const sf::Vector2<T>& v) const {
		using std::hash;
		std::size_t tmp0 = hash<T>()(v.x);
		std::size_t tmp1 = hash<T>()(v.y);
		tmp0 ^= tmp1 + 0x9e3779b9 + (tmp0 << 6) + (tmp0 >> 2);
		return tmp0;
	}
};

float deg2rad(float d);

float rad2deg(float r);

template <typename T>
bool ready(std::shared_future<T>& future) {
	using namespace std::chrono_literals;
	return future.wait_for(-10ms) == std::future_status::ready;
}

template <typename T>
bool ready(std::future<T>& future) {
	using namespace std::chrono_literals;
	return future.wait_for(-10ms) == std::future_status::ready;
}

std::string base64_encode(const std::vector<char>& data);
size_t base64_decode(const std::string& source, void* pdest, size_t dest_size);
}
