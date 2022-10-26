#pragma once

#include <SFML/Graphics.hpp>
#include <vector>

#include "resource.hpp"
#include "tilemap.hpp"
#include "util.hpp"

class moving_tile;

// manages all moving tiles, including collision between eachother
class moving_tile_manager : public sf::Drawable, public sf::Transformable {
public:
	moving_tile_manager(resource& r, tilemap& t);

	void update(sf::Time dt);

	void restart();	  // reset from the beginning

	// all moving tiles that intersect the given aabb
	std::vector<std::pair<sf::Vector2f, tile>> intersects(sf::FloatRect aabb) const;
	// all moving tiles that intersect the given aabb
	std::vector<std::pair<sf::Vector2f, moving_tile>> intersects_raw(sf::FloatRect aabb) const;

private:
	void draw(sf::RenderTarget&, sf::RenderStates) const;

	std::vector<moving_tile> m_tiles;	// all moving tiles

	tilemap& m_tmap;
	resource& m_r;

	// returns true if the contact is valid and the direction should reverse
	bool m_handle_contact(std::vector<std::pair<sf::Vector2f, tile>> contacts);
};

// a tile that bounces back and forth between collision at a fixed rate
class moving_tile : public sf::Drawable, public sf::Transformable {
public:
	/**
	 * @brief moving tile constructor
	 *
	 * @param x the x position of the tile
	 * @param y the y position of the tile
	 * @param m the tilemap
	 * @param r the resource mgr
	 *
	 * @remarks removes the tile from the tilemap on construction
	 */
	moving_tile(int x, int y, tilemap& m, resource& r);

	// retrieve the internal tile instance
	operator tile() const;

	sf::FloatRect get_aabb() const;							// retrieves the bounding box of the tile
	sf::FloatRect get_aabb(float x, float y) const;			// retrieves the bounding box of the tile
	sf::FloatRect get_ghost_aabb() const;					// retrieves the bounding box of the tile for inter-tile collisions
	sf::FloatRect get_ghost_aabb(float x, float y) const;	// retrieves the bounding box of the tile for inter-tile collisions
	sf::FloatRect get_ghost_aabb_x() const;					// retrieves the bounding box of the tile for inter-tile collisions
	sf::FloatRect get_ghost_aabb_y() const;					// retrieves the bounding box of the tile for inter-tile collisions

	static sf::Vector2f size();	  // retrieves the width and height of the aabb
	sf::Vector2f vel() const;	  // retrieve the current velocity
	sf::Vector2f pos() const;	  // retrieve the current position

	enum dir {
		up	  = 0,
		right = 1,
		down  = 2,
		left  = 3
	};

private:
	void draw(sf::RenderTarget&, sf::RenderStates) const;

	sf::Sprite m_spr;

	tile m_t;
	tilemap& m_tmap;
	resource& m_r;

	const struct physics {
		float vel = 2.f;
	} phys;

	dir m_start_dir;

	float m_xp;	  // tile x pos
	float m_yp;	  // tile y pos

	float m_xv;	  // tile x velocity
	float m_yv;	  // tile y velocity

	float m_start_x;   // start x pos
	float m_start_y;   // start y pos

	void m_sync_position();	  // move the sprite's position to the stored one
	void m_restart();

	friend class moving_tile_manager;
};
