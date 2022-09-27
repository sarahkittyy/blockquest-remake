#include "level.hpp"

level::level(resource& r)
	: m_r(r),
	  m_tmap(m_r.tex("assets/tiles.png"), 32, 32, 16) {
}

void level::draw(sf::RenderTarget& t, sf::RenderStates s) const {
	s.transform *= getTransform();
	t.draw(m_tmap, s);
}

nlohmann::json level::serialize() const {
	nlohmann::json j;
	j["tiles"] = m_tmap.serialize();
	return j;
}

void level::deserialize(const nlohmann::json& j) {
	m_tmap.deserialize(j.at("tiles"));
}

tilemap& level::map() {
	return m_tmap;
}

