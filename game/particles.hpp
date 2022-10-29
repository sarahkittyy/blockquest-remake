#pragma once

#include <SFML/Graphics.hpp>
#include <vector>

class particle_system;

/// particle data type
struct particle {
	sf::Time lifetime = sf::seconds(1);	  // particle disappears after this long
	float xp = 0, yp = 0;				  // xyz position, velocity, acceleration
	float xv = 0, yv = 0;
	float xa = 0, ya = 0;
	float alpha = 1, alpha_v = 0, alpha_a = 0;	 // alpha (opacity) from 0-1, with velocity and acceleration to control curve
	float xs = 1, xs_v = 0, xs_a = 0;			 // x and y scaling, with vel and accel
	float ys = 1, ys_v = 0, ys_a = 0;
	int tx = 1, ty = 1;				// texture x and y frame count
	int tc		= 1;				// frames from 0 to play
	sf::Time fs = sf::seconds(1);	// how long between each frame

	// do not touch these
	int m_cframe	   = 0;				   // current animation frame
	sf::Time m_fs_live = sf::seconds(1);   // for timing the animation
};

/// abstract system class, the behavior of the particles and their emission is defined in derived classes
class particle_system : public sf::Drawable, public sf::Transformable {
public:
	particle_system(sf::Texture& t);
	virtual ~particle_system();

	void update(sf::Time dt);

	bool dead() const;			  // are there no more particles?
	int particle_count() const;	  // returns how many particles they count

protected:
	// derived classes call this to generate their particles
	void emit(particle p, sf::Time after = sf::seconds(0));

private:
	void draw(sf::RenderTarget&, sf::RenderStates) const;

	// particles that are set to deploy after a delay
	std::vector<std::pair<sf::Time, particle>> m_particle_queue;
	// active particles
	std::vector<particle> m_particles;

	// texture used for each particle
	sf::Texture& m_t;
	// the size of the texture
	sf::Vector2f m_tsz;
};

/// manages particle systems and allows for them to be spawned easily
class particle_manager : public sf::Drawable, public sf::Transformable {
public:
	~particle_manager();

	/**
	 * @brief spawns a particle system
	 *
	 * @tparam System the type of the system, derived from particle_system
	 * @tparam Args the arguments to pass to the constructor of that system
	 * @return System& a reference to the newly constructed particle system
	 */
	template <typename System, typename... Args>
	System& spawn(Args&&... args) {
		m_systems.push_back(new System(args...));
		System* s = dynamic_cast<System*>(m_systems.back());
		return *s;
	}

	void update(sf::Time dt);

private:
	void draw(sf::RenderTarget&, sf::RenderStates) const;

	// all saved particle systems
	std::vector<particle_system*> m_systems;
};

