#include "gif.hpp"

namespace ImGui {

Gif::Gif(sf::Texture& gif_atlas, int frame_ct, sf::Vector2i frame_sz, int fps)
	: m_atlas(gif_atlas),
	  m_img_sz(frame_sz),
	  m_frame(0),
	  m_frame_ct(frame_ct) {
	m_per_frame = sf::seconds(1.f / float(fps));

	sf::Vector2f img_sz(m_atlas.getSize());
	m_sz.x = frame_sz.x / img_sz.x;
	m_sz.y = frame_sz.y / img_sz.y;

	m_atlas_sz.x = img_sz.x / frame_sz.x;
	m_atlas_sz.y = img_sz.y / frame_sz.y;
}

void Gif::update() {
	if (m_c.getElapsedTime() > m_per_frame) {
		m_c.restart();
		m_frame++;
		if (m_frame >= m_frame_ct) {
			m_frame = 0;
		}
	}
}

sf::Vector2i Gif::image_size() const {
	return m_img_sz;
}

void Gif::draw(sf::Vector2i sz) {
	int x = m_frame % m_atlas_sz.x;
	int y = m_frame / m_atlas_sz.x;
	sf::Vector2f uv;
	uv.x = m_sz.x * x;
	uv.y = m_sz.y * y;

	ImGui::Image(
		reinterpret_cast<ImTextureID>(m_atlas.getNativeHandle()),
		sz,
		uv,
		uv + m_sz);
}

}
