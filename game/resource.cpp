#include "resource.hpp"

#include "context.hpp"

resource::resource()
	: m_window(sf::VideoMode(1600, 928 + 24), "BlockQuest Remake"),
	  m_playing() {

	if (sf::VideoMode::getDesktopMode().width < 1600) {
		m_window.setSize({ 1366, 768 + 26 });
	}

	sf::Image icon;
	icon.loadFromFile("assets/gui/play.png");
	m_window.setIcon(icon.getSize().x, icon.getSize().y, icon.getPixelsPtr());

	load_sound("jump", "assets/sound/jump.flac");
	load_sound("dash", "assets/sound/dash.flac");
	load_sound("gameover", "assets/sound/gameover.flac");
	load_sound("gravityflip", "assets/sound/gravityflip.flac");
	load_sound("wallkick", "assets/sound/wallkick.flac");
	load_sound("victory", "assets/sound/victory.flac");

	load_music("menu_bg", "assets/sound/menu_chiptune.wav");
	load_music("game_bg", "assets/sound/bg1_upbeat.wav");
}

resource& resource::get() {
	static resource instance;
	return instance;
}

sf::Texture& resource::tex(std::string path) {
	if (!m_texs.contains(path)) {
		m_texs[path].loadFromFile(path);
	}
	return m_texs[path];
}

ImTextureID resource::imtex(std::string path) {
	sf::Texture& t = tex(path);
	return reinterpret_cast<ImTextureID>(t.getNativeHandle());
}

sf::Font& resource::font(std::string path) {
	if (!m_fonts.contains(path)) {
		m_fonts[path].loadFromFile(path);
	}
	return m_fonts[path];
}

void resource::load_music(std::string name, std::string path) {
	if (!m_music.contains(name)) {
		m_music[name].openFromFile(path);
	}
	sf::Music& m = m_music[name];
	m.setLoop(true);
}

void resource::play_music(std::string name) {
	if (m_playing && *m_playing == name) {
		return;
	}
	stop_music();
	m_music[name].setVolume(context::get().music_volume());
	m_music[name].play();
	m_playing = name;
}

void resource::stop_music() {
	if (m_playing) {
		m_music[*m_playing].stop();
		m_playing = std::nullopt;
	}
}

void resource::reload_music_volume() {
	if (m_playing) {
		m_music[*m_playing].setVolume(context::get().music_volume());
	}
}

sf::RenderWindow& resource::window() {
	return m_window;
}

const sf::RenderWindow& resource::window() const {
	return m_window;
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
	m_sounds[name].setVolume(context::get().sfx_volume());
	m_sounds[name].play();
}

sf::SoundBuffer& resource::sound_buffer(std::string name) {
	if (!m_soundbufs.contains(name)) {
		throw "a sound of that name was not found!";
	}
	return m_soundbufs[name];
}
