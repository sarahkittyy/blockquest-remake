#pragma once

#include <future>
#include <optional>
#include <stdexcept>

#include "util.hpp"

/// the status of an api call is either inactive, currently fetching, or has result
template <typename Body>
class api_handle {
public:
	api_handle()
		: m_status() {
	}

	void reset() {
		if (m_future.valid()) m_future.get();
		m_status.reset();
	}

	void reset(std::future<Body>&& fut) {
		m_status.reset();
		m_future = std::move(fut);
	}

	bool fetching() const {
		return m_future.valid();
	}

	bool ready() const {
		return m_status.has_value();
	}

	void poll() {
		if (!ready() && m_future.valid() && util::ready(m_future)) {
			m_status = m_future.get();
		}
	}

	Body& get() {
		if (!m_status) throw std::runtime_error("Attempt to fetch api handle response when not ready");
		return *m_status;
	}

	const Body& get() const {
		if (!m_status) throw std::runtime_error("Attempt to fetch api handle response when not ready");
		return *m_status;
	}

private:
	std::future<Body> m_future;
	std::optional<Body> m_status;
};
