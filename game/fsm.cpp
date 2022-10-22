#include "fsm.hpp"

state::state(resource& r)
	: m_r(r) {
}

state::~state() {
}

void state::update(fsm* sm, sf::Time dt) {
}

void state::draw(sf::RenderTarget& t, sf::RenderStates s) const {
	s.transform *= getTransform();
}

void state::imdraw(fsm* sm) {
}

sf::Color state::bg() const {
	return sf::Color(0xC8AD7FFF);
}

resource& state::r() {
	return m_r;
}

// fsm methods //

fsm::fsm(resource& r)
	: m_state(new state(r)),
	  m_saved_state(nullptr),
	  m_r(r) {
}


fsm::~fsm() {
	if (m_saved_state != nullptr) {
		delete m_saved_state;
	}
}


void fsm::update(sf::Time dt) {
	if (m_saved_state != nullptr) {
		m_state.reset(m_saved_state);
		m_saved_state = nullptr;
	}
	m_state->update(this, dt);
}

void fsm::draw(sf::RenderTarget& t, sf::RenderStates s) const {
	s.transform *= getTransform();
	t.draw(*m_state, s);
}

void fsm::imdraw() {
	m_state->imdraw(this);
}

sf::Color fsm::bg() const {
	return m_state->bg();
}
