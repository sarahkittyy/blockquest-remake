#pragma once

#include <imgui.h>

#include "api_handle.hpp"
#include "auth.hpp"

class forgot_password_modal {
public:
	forgot_password_modal();

	void open();
	void imdraw();

	// returns true once password has been reset
	bool success() const;

private:
	static int m_next_id;
	int m_ex_id;   // for imgui id system

	void m_reset();
	void m_open_step_2();

	char m_email[150];
	char m_code[70];
	char m_new_password[70];

	api_handle<auth::forgot_password_response> m_forgot_handle;
	api_handle<auth::reset_password_response> m_reset_handle;
};
