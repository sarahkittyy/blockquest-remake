#include "player.hpp"

#include "world.hpp"

#include <iostream>

player::player(sf::Texture& tex)
	: animated_sprite(tex, 16, 16) {
	add_animation("stand", { 1 });
	add_animation("walk", { 0, 1, 2, 1 });
	add_animation("jump", { 3 });
	add_animation("fall", { 4 });
	add_animation("climb", { 5, 6, 7, 6 });
	add_animation("dash_start", { 11 });
	add_animation("dash", { 8, 9 });

	set_animation("stand");
	set_frame_speed(sf::milliseconds(125));
}
void player::update() {
	animate();
}

void player::draw(sf::RenderTarget& t, sf::RenderStates s) const {
	s.transform *= getTransform();
	animated_sprite::draw(t, s);
}
