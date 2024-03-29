#pragma once

#include <SFML/Graphics.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <iostream>
#include <sstream>
#include <vector>
#include "imgui.h"

struct tile;

/**
 * @brief singleton class to render optional debug data to the screen
 */
class debug : public sf::Drawable, public sf::Transformable {
public:
	static debug& get();
	static debug& log();

	bool ndebug() const {
#ifdef NDEBUG
		return true;
#else
		return !m_open;
#endif
	}

	template <typename T>	// output something to the stream
	debug& operator<<(T val) {
		if (ndebug()) {
			if (m_log_mode)
				std::cout << val;
			return *this;
		}
		if (m_log_mode)
			m_log << val;
		else
			m_data << val;
		return *this;
	}
	template <>
	debug& operator<<(sf::FloatRect);
	template <typename T>
	debug& operator<<(sf::Vector2<T> v) {
		*this << "{"
			  << v.x
			  << ", "
			  << v.y
			  << "}";
		return *this;
	}
	template <>
	debug& operator<<(tile t);
	template <typename T>
	debug& operator<<(std::vector<T> vec) {
		if (vec.size() == 0) {
			*this << "{}";
		} else {
			*this << "{" << vec[0];
			for (int i = 1; i < vec.size(); ++i) {
				*this << ", " << vec[i];
			}
			*this << "}";
		}
		return *this;
	}

	void flush();	// call per frame to reset the debug draw

	debug& box(sf::FloatRect box, sf::Color c = sf::Color(127, 127, 127, 127));

	void imdraw(sf::Time dt);	// render to imgui

private:
	void draw(sf::RenderTarget& t, sf::RenderStates s) const;	// sfml draw

	std::ostringstream m_data;									// the stringstream
	std::ostringstream m_log;									// log stream
	std::vector<std::pair<sf::FloatRect, sf::Color>> m_boxes;	// draw boxes for visualization

	bool m_open		  = true;
	bool m_draw_debug = false;
	bool m_log_mode	  = false;
	bool m_demo_open  = false;

	sf::Clock m_last_dt_reset_clock;   // for resetting m_last_dt
	sf::Time m_last_dt;				   // stores imdraw() dt values so they don't have to be updated every frame, but can be staggered

	debug();
	debug(const debug&)			 = delete;
	void operator=(const debug&) = delete;
};
