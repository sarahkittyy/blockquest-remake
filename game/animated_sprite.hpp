#pragma once

#include <SFML/Graphics.hpp>
#include <string>
#include <unordered_map>
#include <vector>

// abstract animated sprite for precise control over animations
class animated_sprite : public sf::Drawable {
public:
	// initialize with a texture and the size of each frame of the texture
	animated_sprite(sf::Texture& tex, int tx, int ty);
	virtual ~animated_sprite();

	void add_animation(std::string name, std::vector<int> frames);	 // add an animation
	void set_animation(std::string name);							 // set the currently running animation
	// sets this animation to play after all the frames of the current one have expired
	void queue_animation(std::string name);
	// fetch the current anim
	std::string get_animation() const;

	void set_frame_speed(sf::Time df);	 // sets the time to play each frame for

	void animate();	  // update the sprite

	sf::Vector2i size() const;	 // get the sprite's size

	sf::Sprite& spr();	 // get the internal sprite

protected:
	virtual void draw(sf::RenderTarget&, sf::RenderStates) const;

private:
	sf::Sprite m_spr;	// the internal sprite used

	sf::Texture& m_tex;	  // sprite texture map
	int m_tx, m_ty;		  // size of each frame of the texture

	sf::Time m_framespeed;			   // how long each frame stays out
	std::string m_current_animation;   // the currently playing animation
	std::string m_queued_animation;	   // the queued animation

	void m_update_sprite_rect();   // update the sprite texture rect

	sf::Clock m_clock;	 // animation clock
	int m_cframe;		 // current animation frame

	std::unordered_map<std::string, std::vector<int>> m_anims;	 // all defined animations
};
