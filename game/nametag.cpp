#include "nametag.hpp"

#include "resource.hpp"

nametag::nametag() {
	m_text.setFont(resource::get().font("assets/verdana.ttf"));
	m_text.setCharacterSize(30);
	m_text.setFillColor(sf::Color::White);

	m_border.setFillColor(sf::Color(0, 0, 0, 127));
}

void nametag::set_name(std::string name) {
	const float padding = 16.f;
	m_name				= name;

	m_text.setString(m_name);
	m_text.setOrigin(m_text.getLocalBounds().width / 2.f, m_text.getLocalBounds().height / 1.4f);

	m_border.setSize({ m_text.getLocalBounds().width + padding, m_text.getLocalBounds().height + padding });
	m_border.setOrigin(m_border.getSize().x / 2.f, m_border.getSize().y / 2.f);
}

void nametag::draw(sf::RenderTarget& t, sf::RenderStates s) const {
	s.transform *= getTransform();
	t.draw(m_border, s);
	t.draw(m_text, s);
}
