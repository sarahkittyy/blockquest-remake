#include "particles.hpp"

#include "debug.hpp"
#include "util.hpp"

particle_system::particle_system(sf::Texture& t)
	: m_t(t),
	  m_tsz(m_t.getSize()) {
}

particle_system::~particle_system() {
}

void particle_system::update(sf::Time dt) {
	// push queued particles
	for (int i = 0; i < m_particle_queue.size();) {
		std::pair<sf::Time, particle>& pq = m_particle_queue[i];
		if (pq.first <= sf::seconds(0)) {
			m_particles.push_back(pq.second);
			m_particle_queue.erase(m_particle_queue.begin() + i);
		} else {
			i++;
			pq.first -= dt;
		}
	}
	for (int i = 0; i < m_particles.size();) {
		particle& p = m_particles[i];
		p.lifetime -= dt;
		p.xp += p.xv * dt.asSeconds();
		p.yp += p.yv * dt.asSeconds();
		p.xv += p.xa * dt.asSeconds();
		p.yv += p.ya * dt.asSeconds();
		p.alpha += p.alpha_v * dt.asSeconds();
		p.alpha_v += p.alpha_a * dt.asSeconds();
		p.xs += p.xs_v * dt.asSeconds();
		p.ys += p.ys_v * dt.asSeconds();
		p.xs_v += p.xs_a * dt.asSeconds();
		p.ys_v += p.ys_a * dt.asSeconds();
		if (p.lifetime <= sf::seconds(0) || p.alpha <= 0) {
			m_particles.erase(m_particles.begin() + i);
		} else
			i++;
	}
}

bool particle_system::dead() const {
	return m_particles.size() == 0;
}

int particle_system::particle_count() const {
	return m_particles.size();
}

void particle_system::emit(particle p, sf::Time after) {
	m_particle_queue.push_back(std::make_pair(after, p));
}

void particle_system::draw(sf::RenderTarget& t, sf::RenderStates s) const {
	s.transform *= getTransform();
	s.texture = &m_t;
	sf::VertexArray va;
	va.setPrimitiveType(sf::Quads);
	for (const auto& p : m_particles) {
		// normalize tsz
		sf::Vector2f hf = util::normalize(m_tsz);
		hf /= 2.f;
		hf.x *= p.xs;
		hf.y *= p.ys;
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
	debug::get() << "particle systems: " << m_systems.size() << "\n";
	for (int i = 0; i < m_systems.size();) {
		particle_system* ps = m_systems[i];
		ps->update(dt);
		debug::get() << "particle system " << i << ": " << ps->particle_count() << " particle ct\n";
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
