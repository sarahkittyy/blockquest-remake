#pragma once

#include <imgui.h>
#include <SFML/Audio.hpp>
#include <SFML/Graphics.hpp>
#include <memory>
#include <unordered_map>

// holds, manages, and caches resources (sounds, textures, etc)
class resource {
public:
	// singleton
	static resource& get();

	// retrieves the texture at the given image path
	sf::Texture& tex(std::string path);
	// retrieves the font at the given path
	sf::Font& font(std::string path);
	// imgui texture id given the path
	ImTextureID imtex(std::string path);

	// retrieve the app window handle
	sf::RenderWindow& window();
	const sf::RenderWindow& window() const;

	// load a sound & give it a name
	void load_sound(std::string name, std::string path);
	// play the sound with the given name
	void play_sound(std::string name);
	// fetch the sound buffer with the given name
	sf::SoundBuffer& sound_buffer(std::string name);

private:
	std::unordered_map<std::string, sf::Texture> m_texs;
	std::unordered_map<std::string, sf::Font> m_fonts;

	sf::RenderWindow m_window;

	std::unordered_map<std::string, sf::SoundBuffer> m_soundbufs;
	std::unordered_map<std::string, sf::Sound> m_sounds;

	resource();
	resource(const resource&) = delete;
	resource(resource&&)	  = delete;
};
