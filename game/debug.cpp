#include "debug.hpp"

#include <algorithm>
#include <cstring>

#include "imgui.h"
#include "tilemap.hpp"

debug& debug::get() {
	static debug instance;
	instance.m_log_mode = false;
	return instance;
}

debug& debug::log() {
	debug& d	 = get();
	d.m_log_mode = true;
	return d;
}

void debug::imdraw(sf::Time dt) {
	if (ndebug()) return;
	if (m_last_dt == sf::Time::Zero) {
		m_last_dt = dt;
	} else {
		if (m_last_dt_reset_clock.getElapsedTime() > sf::milliseconds(250)) {
			m_last_dt_reset_clock.restart();
			m_last_dt = dt;
		}
	}
	if (m_demo_open)
		ImGui::ShowDemoWindow(&m_demo_open);
	ImGuiWindowFlags flags = ImGuiWindowFlags_None;
	ImGui::SetNextWindowPos(ImVec2(1500, 1000), ImGuiCond_Once);
	ImGui::SetNextWindowSize(ImVec2(420, 800), ImGuiCond_Once);
	ImGui::SetNextWindowCollapsed(true, ImGuiCond_Once);
	char title[100];
	std::snprintf(title, 100, "Debug Tools - %06.2f FPS###Debug", 1.f / m_last_dt.asSeconds());
	ImGui::Begin(title, &m_open, flags);

	ImGui::Checkbox("Toggle debug draw", &m_draw_debug);
	ImGui::Checkbox("Show demo window", &m_demo_open);

	ImGui::TextWrapped("%s", m_data.str().c_str());
	ImGui::TextWrapped("\n-- LOGS --\n%s", m_log.str().c_str());

	ImGui::End();
}


void debug::flush() {
	if (ndebug()) return;
	m_data.str("");
	m_boxes.clear();
}

void debug::draw(sf::RenderTarget& t, sf::RenderStates s) const {
	if (ndebug()) return;
	if (!m_draw_debug) return;
	s.transform *= getTransform();
	std::for_each(m_boxes.begin(), m_boxes.end(), [&t, &s](const auto& pair) {
		sf::RectangleShape shape;
		shape.setFillColor(pair.second);
		shape.setOutlineThickness(0);
		shape.setSize({ pair.first.width, pair.first.height });
		shape.setPosition({ pair.first.left, pair.first.top });
		t.draw(shape, s);
	});
}

debug& debug::box(sf::FloatRect box, sf::Color color) {
	if (ndebug()) return *this;
	m_boxes.push_back(std::make_pair(box, color));
	return *this;
}

template <>
debug& debug::operator<<(sf::FloatRect val) {
	if (ndebug()) return *this;
	*this << "[l="
		  << val.left
		  << ", t="
		  << val.top
		  << ", w="
		  << val.width
		  << ", h="
		  << val.height
		  << "]";
	return *this;
}

template <>
debug& debug::operator<<(tile t) {
	if (ndebug()) return *this;
	*this << "(" << int(t.type) << ")";
	return *this;
}

debug::debug() {
}
