#include "player.hpp"

#include "resource.hpp"
#include "world.hpp"

#include <iostream>

player::player()
	: m_inner(resource::get().tex("assets/player_inner.png"), 64, 64),
	  m_outer(resource::get().tex("assets/player_outer.png"), 64, 64) {
	m_inner.add_animation("stand", { 1 });
	m_inner.add_animation("walk", { 0, 1, 2, 1 });
	m_inner.add_animation("jump", { 3 });
	m_inner.add_animation("fall", { 4 });
	m_inner.add_animation("climb", { 5, 6, 7, 6 });
	m_inner.add_animation("hang", { 5 });
	m_inner.add_animation("dash_start", { 11 });
	m_inner.add_animation("dash", { 8, 9 });

	m_inner.set_animation("stand");
	m_inner.set_frame_speed(sf::milliseconds(125));

	m_outer.add_animation("stand", { 1 });
	m_outer.add_animation("walk", { 0, 1, 2, 1 });
	m_outer.add_animation("jump", { 3 });
	m_outer.add_animation("fall", { 4 });
	m_outer.add_animation("climb", { 5, 6, 7, 6 });
	m_outer.add_animation("hang", { 5 });
	m_outer.add_animation("dash_start", { 11 });
	m_outer.add_animation("dash", { 8, 9 });

	m_outer.set_animation("stand");
	m_outer.set_frame_speed(sf::milliseconds(125));

	m_inner.spr().setColor(sf::Color::White);
	m_outer.spr().setColor(sf::Color::Black);
}

void player::set_outline_color(sf::Color outline) {
	m_outer.spr().setColor(outline);
}

void player::set_fill_color(sf::Color fill) {
	m_inner.spr().setColor(fill);
}

sf::Color player::get_outline_color() {
	return m_outer.spr().getColor();
}

sf::Color player::get_fill_color() {
	return m_inner.spr().getColor();
}

void player::set_animation(std::string name) {
	m_inner.set_animation(name);
	m_outer.set_animation(name);
}

void player::queue_animation(std::string name) {
	m_inner.queue_animation(name);
	m_outer.queue_animation(name);
}

sf::Vector2i player::size() const {
	return m_inner.size();
}

void player::update() {
	m_inner.animate();
	m_outer.animate();
}

void player::draw(sf::RenderTarget& t, sf::RenderStates s) const {
	s.transform *= getTransform();
	t.draw(m_inner, s);
	t.draw(m_outer, s);
}
