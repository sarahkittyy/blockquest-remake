#include "fsm.hpp"

state::state() {
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

void state::process_event(sf::Event e) {
}

sf::Color state::bg() const {
	return sf::Color(0xC8AD7FFF);
}

// fsm methods //

fsm::fsm()
	: m_state(new state()),
	  m_saved_state(nullptr) {
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

void fsm::process_event(sf::Event e) {
	if (m_state)
		m_state->process_event(e);
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
