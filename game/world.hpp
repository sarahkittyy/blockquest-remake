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
#include "replay.hpp"
#include "resource.hpp"
#include "settings.hpp"
#include "sio_client.h"
#include "tilemap.hpp"

// takes in a level and renders it, as well as handles input and logic and physics and all things game-y :3
class world : public sf::Drawable, public sf::Transformable {
public:
	world(level l, std::optional<replay> replay = {});
	~world();

	enum dir {
		up	  = 0,
		right = 1,
		down  = 2,
		left  = 3
	};

	bool update(sf::Time dt);	// true if stepped
	void step(sf::Time dt);		// handles physics stuff
	void process_event(sf::Event e);

	// all variables used for the pre-physics controls
	struct control_vars {
		float xp;
		float yp;
		float xv;
		float yv;
		float sx;
		float sy;
		bool climbing;
		bool dashing;
		bool jumping;
		dir dash_dir;
		dir climbing_facing;
		sf::Time since_wallkick;
		sf::Time time_airborne;

		// unchanged vars
		input_state this_frame;
		input_state last_frame;
		bool grounded;
		dir facing;
		bool against_ladder_left;
		bool against_ladder_right;
		bool can_wallkick_left;
		bool can_wallkick_right;
		bool on_ice;
		bool flip_gravity;
		bool alt_controls;
		bool tile_above;

		// true for a period after wallkicking where we should keep facing & movnig the direction of the kick
		bool is_wallkick_locked() const;
		bool grounded_ago(sf::Time t) const;							 // has a player been grounded in the last t seconds?
		void player_wallkick(dir d, particle_manager* pmgr = nullptr);	 // walljump

		static const control_vars empty;
		sio::message::ptr to_message() const;
		static control_vars from_message(const sio::message::ptr& msg);
	};
	static void run_controls(sf::Time dt, control_vars& v, particle_manager* pmgr = nullptr);

	bool won() const;
	bool lost() const;

	replay& get_replay();
	// returns how long it's been since the start of gameplay / replay playback
	sf::Time get_timer() const;

	bool has_playback() const;

	sf::Vector2f get_player_pos() const;
	sf::Vector2f get_player_vel() const;
	sf::Vector2f get_player_scale() const;
	sf::Vector2i get_player_size() const;
	std::string get_player_anim() const;
	input_state get_player_inputs() const;
	bool get_player_grounded() const;
	control_vars get_player_control_vars() const;

	// physics constants
	static const struct physics {
		float xv_max			= 12.54f;
		float yv_max			= 20.00f;
		float x_accel			= 60.0f;
		float x_decel			= 60.0f;
		float jump_v			= 16.5f;
		float grav				= 60.f;
		float shorthop_factor	= 0.4f;
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

private:
	void draw(sf::RenderTarget&, sf::RenderStates) const;	// sfml draw fn

	void m_init_world();	  // sets up tilemaps, the character, the game, and the b2d physics
	void m_restart_world();	  // puts the player at the start, resets moving obstacles

	void m_check_colors();	 // checks if context colors have changed, and updates

	bool m_alt_controls() const;   // are we using alt controls?

	bool m_has_focus;	// does the window have focus rn

	tilemap m_tmap;	  // tilemap of all static, unmoving tiles.kk, ty

	particle_manager m_pmgr;   // particle manager class

	moving_tile_manager m_mt_mgr;	// manages all moving tiles

	level m_level;	 // the raw level data itself

	sf::Time m_ctime = sf::Time::Zero;
	// for interpolation
	inline float dt_since_step() const {
		return m_ctime.asSeconds();
	}

	// messages shown on victory
	sf::Sprite m_game_clear;
	sf::Sprite m_space_to_retry;
	float m_end_alpha;

	// messages shown on defeat
	sf::Sprite m_game_over;
	sf::RectangleShape m_fadeout;

	// PLAYER DATA //

	control_vars m_cvars;

	player m_player;			// player character
	int m_start_x, m_start_y;	// start position

	bool m_restarted   = false;	  // did the player just restart
	bool m_first_input = false;

	replay m_replay;   // the state of all inputs, each frame
	int m_cstep;	   // current step
	std::optional<replay> m_playback;

	bool m_touched_goal = false;

	bool m_on_ice() const;	 // check touching list for ice

	// dashing produces a rythmic noise that the current update loop is not precise enough to handle
	std::jthread m_dash_sfx_thread;

	void m_update_animation();		 // update the animation state of the player
	void m_player_die();			 // run when the player dies
	void m_player_win();			 // run when the player hits the goal
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

	bool m_player_grounded() const;	  // is the player on solid ground
	bool m_just_jumped() const;
	bool m_player_oob() const;
	bool m_can_player_wallkick(dir d, bool keys_pressed = true) const;	 // can the player wallkick (d = direction of kick)
	bool m_tile_above_player() const;									 // is there a tile directly above the player
	bool m_against_ladder(dir d) const;									 // is there a ladder in the given direction
	dir m_facing() const;												 // which direction is the player facing

	static constexpr dir mirror(dir d);	  // left -> right, up -> down

	static sf::Vector2f player_size();	 // width and height of the full player aabb
};
