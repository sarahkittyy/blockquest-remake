#pragma once

#include <SFML/Graphics.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <algorithm>
#include <functional>

#include "level.hpp"
#include "moving_tile.hpp"
#include "player.hpp"
#include "resource.hpp"
#include "tilemap.hpp"

// takes in a level and renders it, as well as handles input and logic and physics and all things game-y :3
class world : public sf::Drawable, public sf::Transformable {
public:
	world(resource& r, level l);

	void update(sf::Time dt);

private:
	void draw(sf::RenderTarget&, sf::RenderStates) const;	// sfml draw fn

	void m_init_world();	  // sets up tilemaps, the character, the game, and the b2d physics
	void m_restart_world();	  // puts the player at the start, resets moving obstacles

	resource& m_r;	  // resource mgr
	tilemap m_tmap;	  // tilemap of all static, unmoving tiles.kk, ty

	moving_tile_manager m_mt_mgr;	// manages all moving tiles

	level m_level;	 // the raw level data itself

	// PLAYER DATA //

	player m_player;			// player character
	int m_start_x, m_start_y;	// start position

	// physics constants
	const struct physics {
		float xv_max  = 12.54f;
		float yv_max  = 20.00f;
		float x_accel = 60.0f;
		float x_decel = 60.0f;
		float grav	  = 60.f;
		float jump_v  = 17.f;
	} phys;

	// units are in tiles
	float m_xp = 0;	  // player x pos
	float m_yp = 0;	  // player y pos
	float m_xv = 0;	  // player x vel
	float m_yv = 0;	  // player y vel


	enum dir {
		up	  = 0,
		right = 1,
		down  = 2,
		left  = 3
	};

	bool m_flip_gravity = false;   // is gravity flipped?

	void m_update_animation();		 // update the animation state of the player
	void m_player_die();			 // run when the player dies
	bool m_dead = false;			 // used to short circuit logic in the case of a death
	void m_sync_player_position();	 // set the player sprite's position to the internal physics position
	// handle contacts. returns true if a collision occured.
	bool m_handle_contact(float x, float y, std::vector<std::pair<sf::Vector2f, tile>> contacts);
	// pull the first contract that is solid
	std::pair<sf::Vector2f, tile> m_first_solid(std::vector<std::pair<sf::Vector2f, tile>> contacts);

	sf::FloatRect m_get_player_aabb(float x, float y) const;   // retrieve the player's aabb
	sf::FloatRect m_get_player_aabb(float x, float y, dir d) const;
	sf::FloatRect m_get_player_top_aabb(float x, float y) const;
	sf::FloatRect m_get_player_bottom_aabb(float x, float y) const;
	sf::FloatRect m_get_player_left_aabb(float x, float y) const;
	sf::FloatRect m_get_player_right_aabb(float x, float y) const;

	// aabbs that extend a little further out to test if we're up against, but not directly touching the tile
	sf::FloatRect m_get_player_ghost_aabb(float x, float y, dir d) const;
	sf::FloatRect m_get_player_top_ghost_aabb(float x, float y) const;
	sf::FloatRect m_get_player_bottom_ghost_aabb(float x, float y) const;
	sf::FloatRect m_get_player_left_ghost_aabb(float x, float y) const;
	sf::FloatRect m_get_player_right_ghost_aabb(float x, float y) const;

	std::vector<tile> m_touching[4];						// tiles being touched on all four sides of the player
	void m_update_touching();								// update the list of tiles being touched
	std::array<moving_tile*, 4> m_moving_platform_handle;	// moving platform, if we're on one
	void m_update_mp();										// update the moving platforms we're touching
	sf::Vector2f m_mp_player_offset(sf::Time dt) const;		// update the player based on the touched moving platforms

	bool m_player_is_squeezed();   // check if the player is being squeezed

	// all_of, any_of, and none_of for the list of touched tiles
	bool m_test_touching_all(dir d, std::function<bool(tile)> pred) const;
	bool m_test_touching_any(dir d, std::function<bool(tile)> pred) const;
	bool m_test_touching_none(dir d, std::function<bool(tile)> pred) const;

	bool m_player_grounded() const;		// is the player on solid ground
	bool m_tile_above_player() const;	// is there a tile directly above the player

	sf::Vector2f m_player_size() const;	  // width and height of the full player aabb
};
