#pragma once

#include <SFML/Audio.hpp>
#include <SFML/Graphics.hpp>
#include <unordered_map>

// holds, manages, and caches resources (sounds, textures, etc)
class resource {
public:
	// retrieves the texture at the given image path
	sf::Texture& tex(std::string path);
	// retrieves the font at the given path
	sf::Font& font(std::string path);

	// load a sound & give it a name
	void load_sound(std::string name, std::string path);
	// play the sound with the given name
	void play_sound(std::string name);

private:
	std::unordered_map<std::string, sf::Texture> m_texs;
	std::unordered_map<std::string, sf::Font> m_fonts;

	std::unordered_map<std::string, sf::SoundBuffer> m_soundbufs;
	std::unordered_map<std::string, sf::Sound> m_sounds;
};
