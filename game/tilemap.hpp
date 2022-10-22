#pragma once

#include <SFML/Graphics.hpp>
#include <cstring>
#include <ranges>
#include <unordered_map>
#include <vector>

#include "util.hpp"

// bitflags for tile properties
struct tile_props {
	int moving = 0;	  // 0 = stationary, 1 = up, 2 = right, 3 = down, 4 = left
};

class tilemap;

// tile structure
struct tile {
	// ordered enum for tile data in the tilemap
	enum tile_type {
		empty = -1,
		// visible tiles
		begin = 0,
		end,
		block,
		ice,
		black,
		gravity,
		spike,
		ladder,
		// invisible tiles
		stopper,
		erase,
		move_up_bit,
		move_right_bit,
		move_down_bit,
		move_left_bit,
		move_up,
		move_right,
		move_down,
		move_left,
		move_none,
		cursor
	};
	tile(tile_type type = tile_type::empty, tile_props props = tile_props());

	tile_type type;		// tile's type
	tile_props props;	// tile's props

	bool operator==(const tile_type&& type) const;	 // check if a tile is of a given type
	bool operator!=(const tile_type&& type) const;	 // check if a tile is not of a given type
	operator int() const;							 // retrieve the texture index of the tile

	int x() const;	 // x pos of the tile
	int y() const;	 // y pos of the tile

	static std::string description(tile_type type);	  // a user-friendly description of the tile

	bool harmful() const;				// will this tile hurt the player
	bool solid() const;					// can the player walk on this tile
	bool blocks_wallkicks() const;		// will this tile block wallkicks
	bool blocks_moving_tiles() const;	// does this tile block the movement of other moving tiles
	bool movable() const;				// is this tile movable
	bool editor_only() const;			//  tiles only visible in the editor
private:
	friend class tilemap;
	tile(tile_type type, int x, int y);
	int m_x = -1, m_y = -1;
};

// stores and renders all static tiles in a level
class tilemap : public sf::Drawable,
				public sf::Transformable {
public:
	// construct with the tilemap's size in tiles, and the size of a single tile's texture
	tilemap(sf::Texture& tex, int xs, int ys, int ts);

	// returns the dimensions of the area in the tilemap the tile's gfx is stored
	sf::IntRect calculate_texture_rect(tile t) const;

	void set(int x, int y, tile t);			// set a tile
	tile get(int x, int y) const;			// get a tile
	tile get(int i) const;					// get a tile at a given index
	const std::vector<tile>& get() const;	// get all tiles
	void clear(int x, int y);				// set the tile to air
	void clear();							// clear the entire map

	bool in_bounds(sf::Vector2i pos) const;	  // is the given tile pos in bounds

	// all tiles that intersect the given aabb
	std::vector<std::pair<sf::Vector2f, tile>> intersects(sf::FloatRect aabb) const;

	sf::Vector2i size() const;		   // get the map size
	int tile_size() const;			   // get the size of a tile
	sf::Vector2f total_size() const;   // size of the map in pixels
	int count() const;				   // total tile count

	int tile_count(tile::tile_type type) const;				  // how many tiles of a given type are there
	sf::Vector2i find_first_of(tile::tile_type type) const;	  // find the first of a type of tile

	void set_editor_view(bool state);	// toggle editor view

	// save this map to string
	std::string save() const;
	// load this map from a given string
	void load(std::string str);

private:
	// render the map! :3
	void draw(sf::RenderTarget&, sf::RenderStates) const;

	sf::Texture& m_tex;	  // texture to use

	sf::VertexArray m_va;			  // the tilemap vertex cache itself
	sf::VertexArray m_va_editor;	  // additional rendering on top of the tilemap only displayed in editor mode.
	void m_flush_va();				  // fully resets m_va with the cached tile data
	void m_update_quad(int i);		  // sets the quad at the index to the tile value in m_tiles
	void m_set_quad(int i, tile t);	  // sets the quad at the index to the given tile

	bool m_oob(int x, int y) const;	  // check if the given tile x / y is out of bounds
	bool m_oob(int i) const;		  // check if the given tile index is out of bounds

	std::vector<tile> m_tiles;	 // all tiles

	int m_xs, m_ys;	  // dimension of the tilemap in tiles
	int m_ts;		  // dimension of one tile in the tilemap texture

	bool m_editor;	 // if true, renders in editor mode
};
