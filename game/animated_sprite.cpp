#include "animated_sprite.hpp"

animated_sprite::animated_sprite(sf::Texture& tex, int tx, int ty)
	: m_tex(tex),
	  m_tx(tx),
	  m_ty(ty),
	  m_framespeed(sf::milliseconds(250)),
	  m_current_animation("default"),
	  m_cframe(0) {
	m_spr.setTexture(m_tex);

	m_anims["default"] = { 0 };
	m_update_sprite_rect();
}

animated_sprite::~animated_sprite() {
}

void animated_sprite::add_animation(std::string name, std::vector<int> frames) {
	m_anims[name] = frames;
}

void animated_sprite::set_animation(std::string name) {
	m_current_animation = name;
	m_queued_animation	= "";
	if (m_cframe >= m_anims[m_current_animation].size()) {
		m_cframe = m_anims[m_current_animation].size() - 1;
	}
	m_update_sprite_rect();
}

void animated_sprite::queue_animation(std::string name) {
	m_queued_animation = name;
}

void animated_sprite::set_frame_speed(sf::Time df) {
	m_framespeed = df;
}

void animated_sprite::animate() {
	// loop the current frame as necessary
	if (m_clock.getElapsedTime() > m_framespeed) {
		m_clock.restart();

		m_cframe++;
		if (m_cframe >= m_anims[m_current_animation].size()) {
			if (m_queued_animation != "") {
				m_current_animation = m_queued_animation;
				m_queued_animation	= "";
			}
			m_cframe = 0;
		}
	}
	// update the sprite rect
	m_update_sprite_rect();
}

void animated_sprite::m_update_sprite_rect() {
	// current frame index to use
	int cfi = m_anims[m_current_animation][m_cframe];
	// current frame x and y pos in the texture
	int cf_x = cfi % (m_tex.getSize().x / m_tx);
	int cf_y = cfi / (m_tex.getSize().x / m_tx);

	m_spr.setTextureRect(sf::IntRect(
		cf_x * m_tx,
		cf_y * m_ty,
		m_tx,
		m_ty));
}

void animated_sprite::draw(sf::RenderTarget& t, sf::RenderStates s) const {
	// s.transform *= getTransform();
	t.draw(m_spr, s);
}

sf::Sprite& animated_sprite::spr() {
	return m_spr;
}

sf::Vector2i animated_sprite::size() const {
	return { m_tx, m_ty };
}
