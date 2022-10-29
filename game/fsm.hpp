#pragma once

#include <SFML/Graphics.hpp>
#include <functional>
#include <memory>
#include <utility>

class fsm;

/// a state the app can be in. is updated every frame, with a draw function
/// derive this to create states
class state : public sf::Drawable, public sf::Transformable {
public:
	state();
	virtual ~state();

	virtual void update(fsm* sm, sf::Time dt);
	virtual void imdraw(fsm* sm);
	virtual void process_event(sf::Event e);

	// override to change the window's background color
	virtual sf::Color bg() const;

protected:
	virtual void draw(sf::RenderTarget&, sf::RenderStates) const;
};

/// manages the current state of the application
class fsm : public sf::Drawable, public sf::Transformable {
public:
	fsm();
	~fsm();

	// update the current state
	void update(sf::Time dt);

	// process sfml events
	void process_event(sf::Event e);

	// can be used in the middle of state->update() to switch states
	template <typename State, typename... Args>
	void swap_state(Args&&... args) {
		if (m_saved_state != nullptr) {
			delete m_saved_state;
		}
		m_saved_state = new State(args...);
	}

	// called when it's time to render imgui
	void imdraw();

	// retrieves the background color
	sf::Color bg() const;

private:
	void draw(sf::RenderTarget&, sf::RenderStates) const;

	std::unique_ptr<state> m_state;
	state* m_saved_state;
};
