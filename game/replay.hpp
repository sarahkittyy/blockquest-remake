#pragma once

#include <SFML/Graphics.hpp>
#include <cstdint>
#include <cstdlib>

#include "api.hpp"

struct input_state {
	bool left  = false;
	bool right = false;
	bool jump  = false;
	bool dash  = false;
	bool up	   = false;
	bool down  = false;
	operator int() const;
	static input_state from_int(int i);
	bool operator==(const input_state& other) const;
};

class replay {
public:
	replay();
	replay(api::replay rpl);   // load from api replay data

	void reset();
	void push(input_state s);

	input_state get(int step) const;

	void set_user(const char* user);
	void set_created(std::time_t created);
	void set_created_now();
	void set_level_id(int levelid);

	const char* get_user() const;
	float get_time() const;
	std::time_t get_created() const;
	int get_level_id() const;

	size_t size() const;		  // get the amount of input states stored
	size_t serial_size() const;	  // get the amount of bytes required to store the whole compressed replay
	bool alt() const;

	// player colors
	sf::Color fill() const;
	sf::Color outline() const;

	// replay header information
	struct header {
		char version[12];	// the version this replay was made in
		int32_t levelId;	// the level id this was played on, or -1 if none
		int32_t created;	// when was this replay created
		char user[59];		// the user who made this replay
		char alt;			// alt control scheme?
		float time;			// duration in seconds
	};

	// attempts to serialize to the buffer, returns false if not big enough
	bool serialize(char* buf, size_t buf_sz) const;
	void deserialize(char* buf, size_t buf_sz);

	std::string serialize_b64() const;
	void deserialize_b64(std::string b64);

	void save_to_file(std::string path) const;
	void load_from_file(std::string path);

	static const sf::Time timestep;

private:
	std::vector<input_state> m_frames;
	std::optional<api::replay> m_rpl;	// api replay if initialized in that way

	header m_h;

	void m_expand();
};
