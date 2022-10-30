#pragma once

#include <SFML/Graphics.hpp>
#include <unordered_set>
#include <vector>

#include "tilemap.hpp"
#include "util.hpp"

class moving_tile;
class moving_blob;

typedef std::unordered_set<sf::Vector2i, util::vector_hash<int>> pos_set;

// manages all moving tiles, including collision between eachother
class moving_tile_manager : public sf::Drawable, public sf::Transformable {
public:
	moving_tile_manager(tilemap& t);

	void update(sf::Time dt);

	void restart();	  // reset from the beginning

	// all moving tiles that intersect the given aabb
	std::vector<std::pair<sf::Vector2f, tile>> intersects(sf::FloatRect aabb) const;
	// all moving tiles that intersect the given aabb
	std::vector<std::pair<sf::Vector2f, moving_tile>> intersects_raw(sf::FloatRect aabb) const;

private:
	void draw(sf::RenderTarget&, sf::RenderStates) const;

	std::vector<moving_blob> m_blobs;							 // all moving tiles
	std::unordered_map<int, std::vector<int>> m_tile_cohesion;	 // all

	tilemap& m_tmap;

	// returns true if the contact is valid and the direction should reverse
	bool m_handle_contact(std::vector<std::pair<sf::Vector2f, tile>> contacts);
};

// stores many moving tiles
class moving_blob : public sf::Drawable, public sf::Transformable {
public:
	moving_blob(tilemap& m);

	// merge all linked tiles at the given position into this blob, and mark them as checked
	void init(int x, int y, pos_set& checked);

	// all moving tiles of this blob that intersect the given aabb
	void intersects(sf::FloatRect aabb, std::vector<std::pair<sf::Vector2f, tile>>& out) const;
	// all moving tiles of this blob that intersect the given aabb
	void intersects_raw(sf::FloatRect aabb, std::vector<std::pair<sf::Vector2f, moving_tile>>& out) const;

	sf::Vector2f vel() const;
	sf::Vector2f pos() const;
	sf::Vector2f size() const;

	sf::FloatRect get_aabb() const;							// retrieves the bounding box of the blob
	sf::FloatRect get_ghost_aabb() const;					// retrieves the bounding box of the blob for inter-blob collisions
	sf::FloatRect get_aabb(float x, float y) const;			// retrieves the bounding box of the blob
	sf::FloatRect get_ghost_aabb(float x, float y) const;	// retrieves the bounding box of the blob for inter-blob collisions

	void m_sync_position();	  // move the sprite's position to the stored one
	void m_restart();

	enum dir {
		up	  = 0,
		right = 1,
		down  = 2,
		left  = 3
	};

private:
	void draw(sf::RenderTarget&, sf::RenderStates) const;

	tilemap& m_tmap;					// reference to the tilemap we're acting upon
	std::vector<moving_tile> m_tiles;	// the tiles contained in this blob

	dir m_start_dir;
	bool m_initialized;

	int m_min_xp;
	int m_min_yp;
	int m_max_xp;
	int m_max_yp;

	sf::Vector2f m_tpos(int idx) const;	  // get the position of the tile at the given index

	float m_xp;	  // tile x pos
	float m_yp;	  // tile y pos

	float m_xv;	  // tile x velocity
	float m_yv;	  // tile y velocity

	float m_start_x;   // start x pos
	float m_start_y;   // start y pos

	void m_sync_tiles();

	struct physics {
		float vel = 2.f;
	} phys;

	friend class moving_tile_manager;
};

// a tile that bounces back and forth between collision at a fixed rate
class moving_tile : public sf::Drawable,
					public sf::Transformable {
public:
	/**
	 * @brief moving tile constructor
	 *
	 * @param x the x position of the tile
	 * @param y the y position of the tile
	 * @param m the tilemap
	 *
	 * @remarks removes the tile from the tilemap on construction
	 */
	moving_tile(tile t, tilemap& m);

	// retrieve the internal tile instance
	operator tile() const;

	sf::FloatRect get_aabb() const;							// retrieves the bounding box of the tile
	sf::FloatRect get_ghost_aabb() const;					// retrieves the bounding box of the tile for inter-tile collisions
	sf::FloatRect get_aabb(float x, float y) const;			// retrieves the bounding box of the tile
	sf::FloatRect get_ghost_aabb(float x, float y) const;	// retrieves the bounding box of the tile for inter-tile collisions

	static sf::Vector2f size();	  // retrieves the width and height of the aabb
	sf::Vector2f vel() const;	  // velocity of this moving tile
	sf::Vector2f delta() const;	  // pos - last pos
	sf::Vector2f pos() const;	  // velocity of this moving tile

private:
	void draw(sf::RenderTarget&, sf::RenderStates) const;

	int m_start_x;
	int m_start_y;

	float m_xp;
	float m_yp;

	float m_last_xp;
	float m_last_yp;

	float m_xv;
	float m_yv;

	sf::Sprite m_spr;

	tile m_t;

	friend class moving_tile_manager;
	friend class moving_blob;
};
