#pragma once

#include <SFML/Graphics.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <algorithm>
#include <array>
#include <functional>
#include <optional>
#include <thread>

#include "level.hpp"
#include "moving_tile.hpp"
#include "particles.hpp"
#include "player.hpp"
#include "resource.hpp"
#include "settings.hpp"
#include "tilemap.hpp"

// takes in a level and renders it, as well as handles input and logic and physics and all things game-y :3
class world : public sf::Drawable, public sf::Transformable {
public:
	world(resource& r, level l);
	~world();

	void update(sf::Time dt);

	bool won() const;
	bool lost() const;

private:
	void draw(sf::RenderTarget&, sf::RenderStates) const;	// sfml draw fn

	void m_init_world();	  // sets up tilemaps, the character, the game, and the b2d physics
	void m_restart_world();	  // puts the player at the start, resets moving obstacles

	resource& m_r;	  // resource mgr
	tilemap m_tmap;	  // tilemap of all static, unmoving tiles.kk, ty

	particle_manager m_pmgr;   // particle manager class

	moving_tile_manager m_mt_mgr;	// manages all moving tiles

	level m_level;	 // the raw level data itself

	// messages shown on victory
	sf::Sprite m_game_clear;
	sf::Sprite m_space_to_retry;
	float m_end_alpha;

	// messages shown on defeat
	sf::Sprite m_game_over;

	// PLAYER DATA //

	player m_player;			// player character
	int m_start_x, m_start_y;	// start position

	enum dir {
		up	  = 0,
		right = 1,
		down  = 2,
		left  = 3
	};

	// physics constants
	const struct physics {
		float xv_max			= 12.54f;
		float yv_max			= 20.00f;
		float x_accel			= 60.0f;
		float x_decel			= 60.0f;
		float jump_v			= 16.5f;
		float grav				= 60.f;
		float shorthop_factor	= 0.5f;
		float air_control		= 0.4f;
		float dash_xv_max		= 20.f;
		float dash_x_accel		= 120.f;
		float dash_air_control	= 0.2f;
		float wallkick_xv		= 9.5f;
		float wallkick_yv		= 16.0f;
		float ice_friction		= 0.2f;
		float climb_yv_max		= 10.0f;
		float climb_ya			= 180.f;
		float climb_dismount_xv = 6.f;
		int coyote_millis		= 75;
	} phys;

	// units are in tiles
	float m_xp = 0;	  // player x pos
	float m_yp = 0;	  // player y pos
	float m_xv = 0;	  // player x vel
	float m_yv = 0;	  // player y vel

	// were these keys hit this frame?
	bool m_left_this_frame	= false;
	bool m_right_this_frame = false;
	bool m_jump_this_frame	= false;
	bool m_dash_this_frame	= false;
	bool m_up_this_frame	= false;
	bool m_down_this_frame	= false;
	// were these keys hit last frame?
	bool m_left_last_frame	= false;
	bool m_right_last_frame = false;
	bool m_jump_last_frame	= false;
	bool m_dash_last_frame	= false;
	bool m_up_last_frame	= false;
	bool m_down_last_frame	= false;

	bool m_touched_goal = false;

	sf::Time m_time_airborne  = sf::seconds(0);	  // counts the number of frames we're airborne for, for coyote time
	bool m_jumping			  = false;
	bool m_dashing			  = false;
	dir m_dash_dir			  = dir::left;
	sf::Time m_since_wallkick = sf::seconds(999);

	bool m_climbing		  = false;	 // are we scaling a ladder rn
	dir m_climbing_facing = dir::left;

	// true for a period after wallkicking where we should keep facing & movnig the direction of the kick
	bool m_is_wallkick_locked() const;
	bool m_on_ice() const;

	// dashing produces a rythmic noise that the current update loop is not precise enough to handle
	std::jthread m_dash_sfx_thread;

	bool m_flip_gravity = false;   // is gravity flipped?

	void m_update_animation();		 // update the animation state of the player
	void m_player_die();			 // run when the player dies
	void m_player_win();			 // run when the player hits the goal
	void m_player_wallkick(dir d);	 // walljump
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
	sf::FloatRect m_get_player_x_aabb(float x, float y) const;
	sf::FloatRect m_get_player_y_aabb(float x, float y) const;

	// aabbs that extend a little further out to test if we're up against, but not directly touching the tile
	sf::FloatRect m_get_player_ghost_aabb(float x, float y, dir d) const;
	sf::FloatRect m_get_player_top_ghost_aabb(float x, float y) const;
	sf::FloatRect m_get_player_bottom_ghost_aabb(float x, float y) const;
	sf::FloatRect m_get_player_left_ghost_aabb(float x, float y) const;
	sf::FloatRect m_get_player_right_ghost_aabb(float x, float y) const;
	sf::FloatRect m_get_player_x_ghost_aabb(float x, float y) const;
	sf::FloatRect m_get_player_y_ghost_aabb(float x, float y) const;

	std::vector<tile> m_touching[4];									  // tiles being touched on all four sides of the player
	void m_update_touching();											  // update the list of tiles being touched
	std::array<std::optional<moving_tile>, 4> m_moving_platform_handle;	  // moving platform, if we're on one
	void m_update_mp();													  // update the moving platforms we're touching
	sf::Vector2f m_mp_player_offset(sf::Time dt) const;					  // update the player based on the touched moving platforms

	bool m_player_is_squeezed();   // check if the player is being squeezed

	// all_of, any_of, and none_of for the list of touched tiles
	bool m_test_touching_all(dir d, std::function<bool(tile)> pred) const;
	bool m_test_touching_any(dir d, std::function<bool(tile)> pred) const;
	bool m_test_touching_none(dir d, std::function<bool(tile)> pred) const;

	bool m_player_grounded() const;					// is the player on solid ground
	bool m_player_grounded_ago(sf::Time t) const;	// has a player been grounded in the last t seconds?
	bool m_just_jumped() const;
	bool m_player_oob() const;
	bool m_can_player_wallkick(dir d, bool keys_pressed = true) const;	 // can the player wallkick (d = direction of kick)
	bool m_tile_above_player() const;									 // is there a tile directly above the player
	bool m_against_ladder(dir d) const;									 // is there a ladder in the given direction
	dir m_facing() const;												 // which direction is the player facing

	constexpr dir mirror(dir d) const;	 // left -> right, up -> down

	sf::Vector2f m_player_size() const;	  // width and height of the full player aabb
};
