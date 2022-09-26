#include "resource.hpp"

sf::Texture& resource::tex(std::string path) {
	if (!m_texs.contains(path)) {
		m_texs[path].loadFromFile(path);
	}
	return m_texs[path];
}

sf::Font& resource::font(std::string path) {
	if (!m_fonts.contains(path)) {
		m_fonts[path].loadFromFile(path);
	}
	return m_fonts[path];
}

void resource::load_sound(std::string name, std::string path) {
	if (!m_soundbufs.contains(name)) {
		m_soundbufs[name].loadFromFile(path);
		m_sounds[name].setBuffer(m_soundbufs[name]);
	}
}

void resource::play_sound(std::string name) {
	if (!m_sounds.contains(name)) {
		throw "a sound of that name was not found!";
	}
	m_sounds[name].play();
}
