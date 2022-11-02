#include "settings.hpp"

#include <cstdlib>

settings::settings()
	: m_keys({
		  { key::LEFT, sf::Keyboard::Left },
		  { key::RIGHT, sf::Keyboard::Right },
		  { key::UP, sf::Keyboard::Up },
		  { key::DOWN, sf::Keyboard::Down },
		  { key::JUMP, sf::Keyboard::Space },
		  { key::DASH, sf::Keyboard::Down },
	  }) {
}

std::string settings::server_url() const {
	if (const char* env_p = std::getenv("SERVER_URL")) {
		return std::string(env_p);
	} else {
#ifdef SERVER_URL
		return SERVER_URL;
#else
		return "https://bq-r.sushicat.rocks";
#endif
	}
}

void settings::set_key_map(std::unordered_map<key, sf::Keyboard::Key> map) {
	m_keys = map;
}

std::unordered_map<key, sf::Keyboard::Key> settings::get_key_map() const {
	return m_keys;
}

settings& settings::get() {
	static settings instance;
	return instance;
}

void settings::set_key(key k, sf::Keyboard::Key v) {
	m_keys[k] = v;
}

sf::Keyboard::Key settings::get_key(key k) const {
	return m_keys.at(k);
}

std::optional<key> settings::get_key(sf::Keyboard::Key k) const {
	for (auto& [control, key] : m_keys) {
		if (key == k) {
			return control;
		}
	}
	return {};
}

bool settings::key_down(key k) const {
	return sf::Keyboard::isKeyPressed(m_keys.at(k));
}

static const char* key_names[] = {
	"A",
	"B",
	"C",
	"D",
	"E",
	"F",
	"G",
	"H",
	"I",
	"J",
	"K",
	"L",
	"M",
	"N",
	"O",
	"P",
	"Q",
	"R",
	"S",
	"T",
	"U",
	"V",
	"W",
	"X",
	"Y",
	"Z",
	"Num0",
	"Num1",
	"Num2",
	"Num3",
	"Num4",
	"Num5",
	"Num6",
	"Num7",
	"Num8",
	"Num9",
	"Escape",
	"LControl",
	"LShift",
	"LAlt",
	"LSystem",
	"RControl",
	"RShift",
	"RAlt",
	"RSystem",
	"Menu",
	"LBracket",
	"RBracket",
	"Semicolon",
	"Comma",
	"Period",
	"Quote",
	"Slash",
	"Backslash",
	"Tilde",
	"Equal",
	"Hyphen",
	"Space",
	"Enter",
	"Backspace",
	"Tab",
	"PageUp",
	"PageDown",
	"End",
	"Home",
	"Insert",
	"Delete",
	"Add",
	"Subtract",
	"Multiply",
	"Divide",
	"Left",
	"Right",
	"Up",
	"Down",
	"Numpad0",
	"Numpad1",
	"Numpad2",
	"Numpad3",
	"Numpad4",
	"Numpad5",
	"Numpad6",
	"Numpad7",
	"Numpad8",
	"Numpad9",
	"F1",
	"F2",
	"F3",
	"F4",
	"F5",
	"F6",
	"F7",
	"F8",
	"F9",
	"F10",
	"F11",
	"F12",
	"F13",
	"F14",
	"F15",
	"Pause",
	"KeyCount",
	"Dash",
	"BackSpace",
	"BackSlash",
	"SemiColon",
	"Return"
};

const char* key_name(sf::Keyboard::Key k) {
	return key_names[k];
}

const char* key_name(key k) {
	static const char* names[] = {
		"Left",
		"Right",
		"Jump",
		"Dash",
		"Up",
		"Down"
	};
	return names[k];
}
