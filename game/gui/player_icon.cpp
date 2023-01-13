#include "player_icon.hpp"

#include "imgui-SFML.h"
#include "imgui.h"

#include "debug.hpp"

player_icon::player_icon(sf::Color fill, sf::Color outline)
	: player_icon(fill.toInteger(), outline.toInteger()) {
}

player_icon::player_icon(int fill, int outline) {
	m_player.set_animation("jump");
	m_player.setScale(1.0, -1.0);
	m_player.setPosition(0, 64);

	m_player_rt.create(64, 64);

	set_fill_color(fill);
	set_outline_color(outline);
}

void player_icon::set_fill_color(int fill) {
	set_fill_color(sf::Color(fill));
}

void player_icon::set_outline_color(int outline) {
	set_outline_color(sf::Color(outline));
}

void player_icon::set_fill_color(sf::Color fill) {
	m_player.set_fill_color(fill);
	update();
}

void player_icon::set_outline_color(sf::Color outline) {
	m_player.set_outline_color(outline);
	update();
}

void player_icon::set_view(sf::View v) {
	m_player_rt.setView(v);
}

void player_icon::update() {
	sf::RenderStates s;
	s.transform *= getTransform();
	m_player.update();

	m_player_rt.clear(sf::Color::Transparent);
	m_player_rt.draw(m_player);
	m_player_rt.display();
}

void player_icon::imdraw(int xs, int ys) {
	ImGui::Image(m_player_rt.getTexture(), sf::Vector2f(xs, ys));
}
