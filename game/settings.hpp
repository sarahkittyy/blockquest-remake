#pragma once

#include <SFML/Graphics.hpp>
#include <optional>
#include <unordered_map>

// all control keys
enum key {
	LEFT = 0,
	RIGHT,
	JUMP,
	DASH,
	UP,
	DOWN,
	RESTART
};

const char* key_name(sf::Keyboard::Key k);
const char* key_name(key k);

// global app settings (singleton)
class settings {
public:
	static settings& get();

	void set_key(key k, sf::Keyboard::Key v);
	sf::Keyboard::Key get_key(key k) const;
	std::optional<key> get_key(sf::Keyboard::Key k) const;
	bool key_down(key k) const;

	std::string server_url() const;

	std::unordered_map<key, sf::Keyboard::Key> get_key_map() const;
	void set_key_map(std::unordered_map<key, sf::Keyboard::Key> map);

private:
	settings();
	settings(const settings&)		= delete;
	void operator=(const settings&) = delete;

	std::unordered_map<key, sf::Keyboard::Key> m_keys;	 // key mappings
};
