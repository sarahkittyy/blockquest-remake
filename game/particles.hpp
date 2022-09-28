#pragma once

#include <SFML/Graphics.hpp>
#include <vector>

#include "resource.hpp"

struct particle {
	sf::Time lifetime = sf::seconds(1);
	float xp = 0, yp = 0;
	float xv = 0, yv = 0;
	float xa = 0, ya = 0;
	float alpha = 1, alpha_v = 0, alpha_a = 0;
	float xs = 1, xs_v = 0, xs_a = 0;
	float ys = 1, ys_v = 0, ys_a = 0;
};

class particle_system : public sf::Drawable, public sf::Transformable {
public:
	particle_system(sf::Texture& t);
	virtual ~particle_system();

	void update(sf::Time dt);

	bool dead() const;
	int particle_count() const;

protected:
	void emit(particle p, sf::Time after = sf::seconds(0));

private:
	void draw(sf::RenderTarget&, sf::RenderStates) const;

	std::vector<std::pair<sf::Time, particle>> m_particle_queue;
	std::vector<particle> m_particles;

	sf::Texture& m_t;
	sf::Vector2f m_tsz;
};

class particle_manager : public sf::Drawable, public sf::Transformable {
public:
	~particle_manager();

	template <typename System, typename... Args>
	System& spawn(Args&&... args) {
		m_systems.push_back(new System(args...));
		System* s = dynamic_cast<System*>(m_systems.back());
		return *s;
	}

	void update(sf::Time dt);

private:
	void draw(sf::RenderTarget&, sf::RenderStates) const;

	std::vector<particle_system*> m_systems;
};

