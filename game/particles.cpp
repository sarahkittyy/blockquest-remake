#include "particles.hpp"

particle_system::particle_system(sf::Texture& t)
	: m_t(t),
	  m_tsz(m_t.getSize()) {
}

particle_system::~particle_system() {
}

void particle_system::update(sf::Time dt) {
	for (int i = 0; i < m_particles.size();) {
		particle& p = m_particles[i];
		p.lifetime -= dt;
		p.xp += p.xv * dt.asSeconds();
		p.yp += p.yv * dt.asSeconds();
		p.xv += p.xa * dt.asSeconds();
		p.yv += p.ya * dt.asSeconds();
		p.alpha += p.alpha_v * dt.asSeconds();
		p.alpha_v += p.alpha_a * dt.asSeconds();
		if (p.lifetime <= sf::seconds(0) || p.alpha <= 0) {
			m_particles.erase(m_particles.begin() + i);
		} else
			i++;
	}
}

bool particle_system::dead() const {
	return m_particles.size() == 0;
}

void particle_system::emit(particle p) {
	m_particles.push_back(p);
}

void particle_system::draw(sf::RenderTarget& t, sf::RenderStates s) const {
	s.transform *= getTransform();
	s.texture = &m_t;
	sf::VertexArray va;
	va.setPrimitiveType(sf::Quads);
	sf::Vector2f hf(0.5f, 0.5f);
	for (const auto& p : m_particles) {
		va.append(sf::Vertex({ p.xp - hf.x, p.yp - hf.y }, sf::Color(255, 255, 255, p.alpha * 255), { 0, 0 }));
		va.append(sf::Vertex({ p.xp + hf.x, p.yp - hf.y }, sf::Color(255, 255, 255, p.alpha * 255), { m_tsz.x, 0 }));
		va.append(sf::Vertex({ p.xp + hf.x, p.yp + hf.y }, sf::Color(255, 255, 255, p.alpha * 255), { m_tsz.x, m_tsz.y }));
		va.append(sf::Vertex({ p.xp - hf.x, p.yp + hf.y }, sf::Color(255, 255, 255, p.alpha * 255), { 0, m_tsz.y }));
	}
	t.draw(va, s);
}

/////////////////////////////////////////////////////////////////////////////////////////

particle_manager::~particle_manager() {
	for (auto& sp : m_systems) {
		delete sp;
	}
}

void particle_manager::update(sf::Time dt) {
	for (int i = 0; i < m_systems.size();) {
		particle_system* ps = m_systems[i];
		ps->update(dt);
		if (ps->dead()) {
			m_systems.erase(m_systems.begin() + i);
		} else
			i++;
	}
}

void particle_manager::draw(sf::RenderTarget& t, sf::RenderStates s) const {
	s.transform *= getTransform();
	for (auto& sp : m_systems) {
		t.draw(*sp, s);
	}
}
