#include "replay.hpp"

#include <cmath>
#include <cstring>

#include "util.hpp"

#define CHECK_BIT(v, p) ((v) & (1 << (p)))

const sf::Time replay::timestep = sf::milliseconds(10);

replay::replay() {
	m_frames.reserve(100);
	reset();
}

replay::replay(api::replay rpl)
	: replay() {
	deserialize_b64(rpl.replay);
}

void replay::m_expand() {
	m_frames.reserve(m_frames.capacity() * 2);
}

void replay::reset() {
	std::memset((void*)(&m_h), 0, sizeof(header));
	std::strcpy(m_h.version, api::get().version());
	m_frames.clear();
}

void replay::push(input_state s) {
	m_frames.push_back(s);
	if (m_frames.size() > m_frames.capacity() - 1) {
		m_expand();
	}
}

input_state replay::get(int step) const {
	return m_frames.at(step);
}

void replay::set_user(const char* user) {
	strcpy(m_h.user, user);
}

void replay::set_created(std::time_t created) {
	m_h.created = created;
}

void replay::set_created_now() {
	time(reinterpret_cast<std::time_t*>(&m_h.created));
}

void replay::set_level_id(int levelid) {
	m_h.levelId = levelid;
}

const char* replay::get_user() const {
	return m_h.user;
}

float replay::get_time() const {
	return size() * timestep.asSeconds();
}

std::time_t replay::get_created() const {
	return m_h.created;
}

int replay::get_level_id() const {
	return m_h.levelId;
}

size_t replay::size() const {
	return m_frames.size();
}

size_t replay::serial_size() const {
	// 6 bits per state, 8 bits per byte, with some padding
	// 4 input states = 3 bytes
	return sizeof(header) + int(std::ceil(size() / 4.f) * 3);
}

bool replay::serialize(char* buf, size_t buf_sz) const {
	if (buf_sz < serial_size()) {
		return false;
	}
	replay::header h = m_h;
	h.time			 = get_time();
	// serialize the header first
	std::memcpy(buf, (void*)(&h), sizeof(header));
	buf += sizeof(header);
	// 4 input_states = 24 bits = 3 bytes
	char bit1, bit2, bit3;
	for (int i = 0; i <= size() / 4; i++) {
		bit1 = bit2 = bit3 = 0;
		int idx			   = i * 4;
		input_state i1	   = idx + 0 < size() ? m_frames[idx + 0] : input_state();
		input_state i2	   = idx + 1 < size() ? m_frames[idx + 1] : input_state();
		input_state i3	   = idx + 2 < size() ? m_frames[idx + 2] : input_state();
		input_state i4	   = idx + 3 < size() ? m_frames[idx + 3] : input_state();
		bit1 |= i1.left ? (1UL << 0) : 0;
		bit1 |= i1.right ? (1UL << 1) : 0;
		bit1 |= i1.jump ? (1UL << 2) : 0;
		bit1 |= i1.dash ? (1UL << 3) : 0;
		bit1 |= i1.up ? (1UL << 4) : 0;
		bit1 |= i1.down ? (1UL << 5) : 0;
		bit1 |= i2.left ? (1UL << 6) : 0;
		bit1 |= i2.right ? (1UL << 7) : 0;

		bit2 |= i2.jump ? (1UL << 0) : 0;
		bit2 |= i2.dash ? (1UL << 1) : 0;
		bit2 |= i2.up ? (1UL << 2) : 0;
		bit2 |= i2.down ? (1UL << 3) : 0;
		bit2 |= i3.left ? (1UL << 4) : 0;
		bit2 |= i3.right ? (1UL << 5) : 0;
		bit2 |= i3.jump ? (1UL << 6) : 0;
		bit2 |= i3.dash ? (1UL << 7) : 0;

		bit3 |= i3.up ? (1UL << 0) : 0;
		bit3 |= i3.down ? (1UL << 1) : 0;
		bit3 |= i4.left ? (1UL << 2) : 0;
		bit3 |= i4.right ? (1UL << 3) : 0;
		bit3 |= i4.jump ? (1UL << 4) : 0;
		bit3 |= i4.dash ? (1UL << 5) : 0;
		bit3 |= i4.up ? (1UL << 6) : 0;
		bit3 |= i4.down ? (1UL << 7) : 0;

		*buf = bit1;
		buf++;
		*buf = bit2;
		buf++;
		*buf = bit3;
		buf++;
	}
	return true;
}

void replay::deserialize(char* buf, size_t buf_sz) {
	reset();
	// deserialize the header first
	std::memcpy((void*)(&m_h), buf, sizeof(header));
	buf += sizeof(header);
	buf_sz -= sizeof(header);
	// read 3 bytes at a time, fetching input
	for (int i = 0; i < buf_sz; i += 3) {
		char bit1 = *buf++;
		char bit2 = *buf++;
		char bit3 = *buf++;
		input_state i1, i2, i3, i4;
		i1.left	 = CHECK_BIT(bit1, 0);
		i1.right = CHECK_BIT(bit1, 1);
		i1.jump	 = CHECK_BIT(bit1, 2);
		i1.dash	 = CHECK_BIT(bit1, 3);
		i1.up	 = CHECK_BIT(bit1, 4);
		i1.down	 = CHECK_BIT(bit1, 5);

		i2.left	 = CHECK_BIT(bit1, 6);
		i2.right = CHECK_BIT(bit1, 7);
		i2.jump	 = CHECK_BIT(bit2, 0);
		i2.dash	 = CHECK_BIT(bit2, 1);
		i2.up	 = CHECK_BIT(bit2, 2);
		i2.down	 = CHECK_BIT(bit2, 3);

		i3.left	 = CHECK_BIT(bit2, 4);
		i3.right = CHECK_BIT(bit2, 5);
		i3.jump	 = CHECK_BIT(bit2, 6);
		i3.dash	 = CHECK_BIT(bit2, 7);
		i3.up	 = CHECK_BIT(bit3, 0);
		i3.down	 = CHECK_BIT(bit3, 1);

		i4.left	 = CHECK_BIT(bit3, 2);
		i4.right = CHECK_BIT(bit3, 3);
		i4.jump	 = CHECK_BIT(bit3, 4);
		i4.dash	 = CHECK_BIT(bit3, 5);
		i4.up	 = CHECK_BIT(bit3, 6);
		i4.down	 = CHECK_BIT(bit3, 7);

		push(i1);
		push(i2);
		push(i3);
		push(i4);
	}
}

std::string replay::serialize_b64() const {
	std::vector<char> buf;
	buf.resize(serial_size());
	serialize(buf.data(), serial_size());
	return util::base64_encode(buf);
}

void replay::deserialize_b64(std::string b64) {
	std::string replay;
	replay.resize(b64.size() * 1.2f);
	size_t replay_sz = util::base64_decode(b64, replay.data(), replay.capacity());
	deserialize(replay.data(), replay_sz);
}

void replay::save_to_file(std::string path) const {
	char* buf = new char[serial_size()];
	serialize(buf, serial_size());
	std::ofstream file(path, std::ios::out | std::ios::binary);
	if (!file) throw std::runtime_error("Could not open " + path + " for write.");
	file.write(buf, serial_size());
	file.close();
}

void replay::load_from_file(std::string path) {
	std::ifstream file(path, std::ios::ate | std::ios::binary | std::ios::in);
	if (!file) throw std::runtime_error("Could not open " + path + " for reading.");
	std::streamsize sz = file.tellg();
	file.seekg(0, std::ios::beg);
	char* buf = new char[sz];
	file.read(buf, sz);
	file.close();
	deserialize(buf, sz);
	delete[] buf;
}
