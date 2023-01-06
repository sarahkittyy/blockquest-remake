#include "edit.hpp"

#include "imgui-SFML.h"
#include "imgui.h"
#include "imgui_internal.h"

#include "ImGuiFileDialog.h"
#include "gui/menu_bar.hpp"

#include <cstring>
#include <ctime>
#include <ios>
#include <stdexcept>

#include "../debug.hpp"
#include "../gui/editor_tile_button.hpp"
#include "../gui/image_text_button.hpp"

#include "context.hpp"
#include "states/search.hpp"

namespace states {

edit::edit(api::level lvl)
	: edit() {
	m_load_api_level(lvl);
}

edit::edit(api::level lvl, replay rpl)
	: edit() {
	m_loaded_replay = rpl;
	m_load_api_level(lvl);
}

edit::edit()
	: m_menu_bar(),
	  m_cursor(),
	  m_border(resource::get().tex("assets/tiles.png"), 34, 32, 64),
	  m_level_size(1024),
	  m_cursor_type(PENCIL),
	  m_tiles(resource::get().tex("assets/tiles.png")),
	  m_tools(resource::get().tex("assets/gui/tools.png")),
	  m_stroke_start(-1, -1),
	  m_stroke_active(false),
	  m_pencil_active(false),
	  m_stroke_map(resource::get().tex("assets/tiles.png"), 32, 32, 64),
	  m_old_mouse_tile(-1, -1),
	  m_last_placed(-1, -1) {

	resource::get().play_music("menu_bg");

	// propagate an initial resize
	sf::Event rsz_evt;
	rsz_evt.type		= sf::Event::Resized;
	rsz_evt.size.width	= resource::get().window().getSize().x;
	rsz_evt.size.height = resource::get().window().getSize().y;
	process_event(rsz_evt);

	m_timer_text.setFont(resource::get().font("assets/verdana.ttf"));
	m_timer_text.setCharacterSize(24);
	m_timer_text.setFillColor(sf::Color::Black);
	m_timer_text.setOutlineThickness(1);
	m_timer_text.setOutlineColor(sf::Color(0x964B00));
	m_timer_text.setString("00:00");
	m_timer_text.setOrigin(m_timer_text.getLocalBounds().width / 2.f, 0);
	m_timer_text.setPosition(34 * 64 / 2.f, 16.f);

	m_rt.create(34 * 64, 32 * 64);
	m_map.setTexture(m_rt.getTexture());

	std::memset(m_title_buffer, 0, 50);
	std::memset(m_description_buffer, 0, 50);
	m_id = 0;

	m_update_transforms();

	m_level().map().set_editor_view(true);
	m_cursor.map().set_editor_view(true);
	m_border.set_editor_view(true);
	m_stroke_map.set_editor_view(true);

	m_bg.setTexture(resource::get().tex("assets/bg.png"));
	m_bg.setColor(sf::Color(255, 255, 255, 128));
	m_bg.setOrigin(m_bg.getLocalBounds().width / 2.f, m_bg.getLocalBounds().height / 2.f);
	m_bg.rotate(33.f);
	m_bg.setScale(4.f, 4.f);
	m_bg.setPosition(sf::Vector2f(resource::get().window().getSize()) * 0.5f);

	for (int i = 0; i < 32; ++i) {
		m_border.set(0, i, tile::block);
		m_border.set(33, i, tile::block);
	}

	if (!m_test_playing())
		m_update_mouse_tile();
}

edit::~edit() {
	debug::get().setScale(1.f, 1.f);
	debug::get().setPosition(0, 0);

	if (m_test_playing()) {
		m_toggle_test_play();
	}
}

level& edit::m_level() {
	return context::get().editor_level();
}

const level& edit::m_level() const {
	return context::get().editor_level();
}

float edit::m_level_scale() const {
	return m_level_size / m_level().map().total_size().y;
}

void edit::m_update_transforms() {
	sf::Vector2f win_sz(resource::get().window().getSize());

	float scale = m_level_scale();

	m_level().setPosition(64, 0);
	m_cursor.setPosition(64, 0);
	m_stroke_map.setPosition(64, 0);
	if (m_test_play_world) {
		m_test_play_world->setPosition(64, 0);
	}

	m_map.setOrigin(m_rt.getSize().x / 2.f, -24 / scale);
	m_map.setPosition(win_sz.x / 2.f, 0);
	m_map.setScale(scale, scale);

	debug::log() << scale << "\n";

	debug::get().setOrigin(m_rt.getSize().x / 2.f, -24 / scale);
	debug::get().setPosition(win_sz.x / 2.f + 64 * scale, 0);
	debug::get().setScale(scale, scale);
}

void edit::process_event(sf::Event e) {
	m_menu_bar.process_event(e);
	if (m_test_play_world) {
		m_test_play_world->process_event(e);
	}
	switch (e.type) {
	default:
		break;
	case sf::Event::Resized:
		m_level_size = std::min(e.size.width - 2 * int(m_level().map().tile_size() * m_level_scale()), e.size.height - 24);
		m_update_transforms();
		break;
	}
}

bool edit::m_mouse_pressed(sf::Mouse::Button btn) const {
	return sf::Mouse::isButtonPressed(btn) && resource::get().window().hasFocus();
}

void edit::update(fsm* sm, sf::Time dt) {
	sf::Vector2i mouse_tile = m_update_mouse_tile();

	debug::get() << auth::get().username() << " (" << auth::get().tier() << ")\n";

	if (m_undo_queue.size() > UNDO_MAX) {
		m_undo_queue.pop_front();
	}
	if (m_redo_queue.size() > UNDO_MAX) {
		m_redo_queue.pop_front();
	}

	m_upload_handle.poll();
	m_download_handle.poll();

	// if we're not on gui, and on the map, and ready to place a tile
	if (!ImGui::GetIO().WantCaptureMouse &&
		m_mouse_pressed(sf::Mouse::Left) &&
		!m_test_playing() &&
		m_level().map().in_bounds(mouse_tile)) {
		// metadata check
		if (m_level().has_metadata() && !m_is_current_level_ours())
			m_level().clear_metadata();

		// set the tile
		switch (m_cursor_type) {
		case PENCIL: {
			if (!m_pencil_active) {
				m_pencil_active = true;
				m_pencil_undo_queue.clear();
			}
			auto diff = m_set_tile(mouse_tile, m_selected_tile, m_info_msg);
			if (diff && m_last_placed != mouse_tile && !diff->same()) {
				m_pencil_undo_queue.push_back(*diff);
			}
			break;
		}
		case FLOOD: {
			auto diffs = m_flood_fill(mouse_tile, m_level().map().get(mouse_tile.x, mouse_tile.y), m_info_msg);
			if (!diffs.empty()) {
				m_undo_queue.push_back(diffs);
			}
			break;
		}
		case STROKE:
			if (m_selected_tile == tile::begin || m_selected_tile == tile::end) {
				auto diff = m_set_tile(mouse_tile, m_selected_tile, m_info_msg);
				if (diff && m_last_placed != mouse_tile && !diff->same()) {
					m_undo_queue.push_back({ *diff });
				}
			} else
				m_stroke_fill(mouse_tile, m_info_msg);
			break;
		}
		m_last_placed = mouse_tile;
	} else {
		// if we're done stroking, render the line
		if (m_stroke_active) {
			m_stroke_active = false;
			auto diffs		= m_stroke_map.layer_over(m_level().map(), true);
			if (!diffs.empty()) {
				m_undo_queue.push_back(diffs);
			}
			m_stroke_map.clear();
		}
		if (m_pencil_active) {
			m_pencil_active = false;
			m_undo_queue.push_back(m_pencil_undo_queue);
			m_pencil_undo_queue.clear();
		}
	}

	if (m_test_playing()) {
		bool stepped = m_test_play_world->update(dt);
		if (stepped) {
			float time = m_test_play_world->get_timer().asSeconds();
			int whole  = std::floor(time);
			int rest   = std::floor((time - whole) * 100.f);
			std::ostringstream ss;
			ss << whole << "." << rest;
			m_timer_text.setString(ss.str());
		}
	}

	// DRAW
	m_rt.clear(bg());
	m_rt.draw(m_border);
	if (!m_test_playing()) {
		m_rt.draw(m_level());
		m_rt.draw(m_stroke_map);
		m_rt.draw(m_cursor);
	} else {
		m_rt.draw(*m_test_play_world);
		m_rt.draw(m_timer_text);
	}
	m_rt.display();
}

std::vector<tilemap::diff> edit::m_stroke_fill(sf::Vector2i pos, std::string& error) {
	// exclude certain types
	switch (m_selected_tile) {
	default:
		break;
	case tile::move_up:
	case tile::move_down:
	case tile::move_left:
	case tile::move_right:
	case tile::move_none:
	case tile::move_up_bit:
	case tile::move_right_bit:
	case tile::move_down_bit:
	case tile::move_left_bit:
	case tile::cursor:
	case tile::begin:
	case tile::end:
		return {};
	}
	static sf::Vector2i last_pos;
	if (!m_stroke_active) {
		m_stroke_active = true;
		m_stroke_start	= pos;
		last_pos		= pos;
	}
	// clear the previous line
	m_stroke_map.set_line(m_stroke_start, last_pos, tile::empty);
	// for straight lines
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift) || sf::Keyboard::isKeyPressed(sf::Keyboard::RShift)) {
		sf::Vector2i dist = pos - m_stroke_start;
		dist.x			  = std::abs(dist.x);
		dist.y			  = std::abs(dist.y);
		if (dist.x > dist.y) {
			pos.y = m_stroke_start.y;
		} else {
			pos.x = m_stroke_start.x;
		}
	}
	last_pos = pos;
	return m_stroke_map.set_line(m_stroke_start, pos, selected());
}

tile edit::selected() const {
	tile t(m_selected_tile);
	if (t.movable() && m_selected_direction != tile::move_none) {
		t.props = { .moving = int(m_selected_direction) - 13 };
	}
	return t;
}

std::optional<tilemap::diff> edit::m_set_tile(sf::Vector2i pos, tile::tile_type type, std::string& error) {
	try {
		std::optional<tilemap::diff> d;

		tilemap& m	= m_level().map();
		const int x = pos.x;
		const int y = pos.y;
		switch (type) {
		case tile::block:
		case tile::ice:
		case tile::black:
		case tile::gravity:
		case tile::spike:
		case tile::ladder:
		case tile::stopper:
			d = m.set(x, y, selected());
			break;
		case tile::empty:
		case tile::erase:
			d = m.clear(x, y);
			break;
		case tile::move_up:
		case tile::move_down:
		case tile::move_left:
		case tile::move_right:
		case tile::move_none:
		case tile::move_up_bit:
		case tile::move_right_bit:
		case tile::move_down_bit:
		case tile::move_left_bit:
		case tile::cursor:
			throw std::runtime_error("Tile for internal use, cannot be placed.");
		case tile::begin:
		case tile::end:
			if (m.tile_count(type) != 0) {
				sf::Vector2i to_remove = m.find_first_of(type);
				m.clear(to_remove.x, to_remove.y);
			}
			d = m.set(x, y, selected());
			break;
		default:
			return {};
		}
		return d;
	} catch (const std::runtime_error& e) {
		error = e.what();
		return {};
	}
}

std::vector<tilemap::diff> edit::m_flood_fill(sf::Vector2i pos, tile replacing, std::string& error) {
	try {
		tilemap& m	= m_level().map();
		const int x = pos.x;
		const int y = pos.y;
		tile t		= m.get(x, y);
		tile sel	= selected();
		// do not set this tile if these conditions are met
		if (x < 0 || x >= m.size().x || y < 0 || y >= m.size().y) return {};
		if (!t.eq(replacing)) return {};
		if (t.eq(sel)) return {};
		if (sel.eq(replacing)) return {};
		if (t == tile::empty && sel.type == tile::erase) return {};
		switch (sel.type) {
		case tile::move_up:
		case tile::move_down:
		case tile::move_left:
		case tile::move_right:
		case tile::move_none:
		case tile::move_up_bit:
		case tile::move_right_bit:
		case tile::move_down_bit:
		case tile::move_left_bit:
		case tile::cursor:
		case tile::begin:
		case tile::end:
			return {};
		default:
			break;
		}
		////////////////////////////////////////////////////

		std::vector<tilemap::diff> ret;
		auto d = m_set_tile(pos, sel.type, error);
		if (d) {
			ret.push_back(*d);
			auto ret1 = m_flood_fill({ x - 1, y }, replacing, error);
			auto ret2 = m_flood_fill({ x + 1, y }, replacing, error);
			auto ret3 = m_flood_fill({ x, y - 1 }, replacing, error);
			auto ret4 = m_flood_fill({ x, y + 1 }, replacing, error);
			ret.reserve(ret1.size() + ret2.size() + ret3.size() + ret4.size() + 1);
			ret.insert(ret.end(), ret1.begin(), ret1.end());
			ret.insert(ret.end(), ret2.begin(), ret2.end());
			ret.insert(ret.end(), ret3.begin(), ret3.end());
			ret.insert(ret.end(), ret4.begin(), ret4.end());
		}
		return ret;
	} catch (const std::runtime_error& e) {
		error = e.what();
		return {};
	}
}

sf::Vector2i edit::m_update_mouse_tile() {
	sf::Vector2f mouse_pos(sf::Mouse::getPosition(resource::get().window()));
	mouse_pos = m_map.getInverseTransform().transformPoint(mouse_pos);
	mouse_pos = m_level().getInverseTransform().transformPoint(mouse_pos);

	const sf::Vector2i mouse_tile = m_level().mouse_tile(mouse_pos);
	// debug::get() << mouse_tile << "\n";
	if (mouse_tile != m_old_mouse_tile) {
		m_last_placed = { -1, -1 };
		m_cursor.map().clear(m_old_mouse_tile.x, m_old_mouse_tile.y);
		m_old_mouse_tile = mouse_tile;
		if (m_cursor.map().in_bounds(mouse_tile)) {
			m_cursor.map().set(mouse_tile.x, mouse_tile.y, tile::cursor);
		}
	}
	return mouse_tile;
}

void edit::imdraw(fsm* sm) {
	// clang-format off
	ImGuiWindowFlags flags = ImGuiWindowFlags_None;
	flags |= ImGuiWindowFlags_NoResize;
	flags |= ImGuiWindowFlags_AlwaysAutoResize;

	sf::Vector2i wsz(resource::get().window().getSize());

	// menu bar
	m_menu_bar.imdraw(m_info_msg, sm);

	// controls
	ImGui::SetNextWindowPos(ImVec2(50, 50), ImGuiCond_FirstUseEver);
	ImGui::Begin("Controls", nullptr, flags);
	m_gui_controls(sm);
	ImGui::End();

	// block picker
	ImGui::SetNextWindowPos(ImVec2(15, wsz.y / 2.f), ImGuiCond_FirstUseEver);
	ImGui::Begin("Blocks", nullptr, flags);
	m_gui_block_picker(sm);
	ImGui::End();

	ImGui::SetNextWindowPos(ImVec2(wsz.x - 250, 50), ImGuiCond_FirstUseEver);
	ImGui::Begin("Menu", nullptr, flags);
	m_gui_menu(sm);
	ImGui::End();

	if (m_level().has_metadata()) {
		// level info
		ImGui::SetNextWindowPos(ImVec2(wsz.x - 300, wsz.y / 2.f), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(ImVec2(300, 500), ImGuiCond_Once);
		ImGui::Begin("Level Info");
		m_gui_level_info(sm);
		ImGui::End();

		// victory  
		if (m_test_play_world && m_test_play_world->won() && !m_test_play_world->has_playback()) {
			ImGui::SetNextWindowPos(ImVec2(wsz.x / 2.f - 150, wsz.y / 2.f + 75), ImGuiCond_Appearing);
			ImGui::SetNextWindowSize(ImVec2(300, 250), ImGuiCond_Appearing);
			ImGui::Begin("Replay");
			m_gui_replay_submit(sm);
			ImGui::End();
		}
	}

}

bool edit::m_is_current_level_ours() const {
	if (!m_level().has_metadata()) return true;
	if (!auth::get().authed()) return false;
	if (m_level().get_metadata().author != auth::get().username()) return false;
	return true;
}

void edit::m_load_api_level(api::level lvl) {
	m_upload_handle.reset();
	m_download_handle.reset();
	m_level().load_from_api(lvl);
	api::level md = m_level().get_metadata();
	if (m_is_current_level_ours()) {
		std::strncpy(m_title_buffer, m_level().get_metadata().title.c_str(), 50);
		std::strncpy(m_description_buffer, m_level().get_metadata().description.c_str(), 256);
	} else {
		std::memset(m_title_buffer, 0, 50);
		std::memset(m_description_buffer, 0, 50);
	}
	m_id = md.id;
}

void edit::m_clear_level() {
	m_upload_handle.reset();
	m_download_handle.reset();
	m_level().clear();
	std::memset(m_title_buffer, 0, 50);
	std::memset(m_description_buffer, 0, 50);
	m_id = 0;
}

void edit::m_gui_replay_submit(fsm* sm) {
	if (!m_test_play_world) return;
	m_upload_replay_handle.poll();

	auto& md = m_level().get_metadata();
	replay& rp = m_test_play_world->get_replay();
	bool authed = auth::get().authed();
	bool can_submit = !m_upload_replay_handle.fetching() && authed && ((md.myRecord && md.myRecord->time >= rp.get_time()) || !md.myRecord);
	
	sf::Vector2f wsz(resource::get().window().getSize());
	
	if (!m_upload_replay_handle.fetching() && m_upload_replay_handle.ready()) {
		auto& res = m_upload_replay_handle.get();
		if (!res.success) {
			ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetColorU32(sf::Color::Red));
			ImGui::TextWrapped("%s", res.error->c_str());
			ImGui::PopStyleColor();
		} else {
			if (m_level().has_metadata()) {
				// old level records
				auto& md = m_level().get_metadata();
				md.myRecord = {
					.user = auth::get().username(),
					.time = std::min<float>(md.myRecord ? md.myRecord->time : 99.9999f, rp.get_time())
				};
				// if our new record is better than the wr, we have the new wr
				if (md.myRecord->time < *res.newBest) {
					md.record = md.myRecord;
				}
			}

			return m_toggle_test_play();
		}
	}

	if (md.record)
		ImGui::TextWrapped("Time: %.2f (WR: %.2fs by %s)", rp.get_time(), md.record->time, md.record->user.c_str());
	else
		ImGui::TextWrapped("Time: %.2f", rp.get_time());

	if (md.myRecord)
		ImGui::TextWrapped("Your record: %.2f", md.myRecord->time);

	ImGui::TextWrapped("Inputs: %lu", rp.size());

	std::string submit_label = std::string(m_upload_replay_handle.fetching() ? "Submitting..." : "Submit") + "###SubmitRecord";
	ImGui::BeginDisabled(!can_submit);
	if (ImGui::ImageButtonWithText(resource::get().imtex("assets/gui/upload.png"), submit_label.c_str())) {
		rp.set_created_now();
		rp.set_level_id(md.id);
		rp.set_user(auth::get().username().c_str());
		m_upload_replay_handle.reset(api::get().upload_replay(rp));
	}
	ImGui::SameLine();
	if (ImGui::ImageButtonWithText(resource::get().imtex("assets/gui/download.png"), "Download###DownloadRecord")) {
		ImGuiFileDialog::Instance()->OpenDialog("DownloadReplayDialog", "Download Replay", ".rpl", ".", 1, nullptr, ImGuiFileDialogFlags_Modal);
	}
	if (ImGuiFileDialog::Instance()->Display("DownloadReplayDialog", ImGuiWindowFlags_NoCollapse, wsz * 0.5f)) {
		if (ImGuiFileDialog::Instance()->IsOk()) {
			std::string path = ImGuiFileDialog::Instance()->GetFilePathName();
			rp.set_created_now();
			rp.set_level_id(md.id);
			rp.set_user(auth::get().username().c_str());
			rp.save_to_file(path);
		}
		ImGuiFileDialog::Instance()->Close();
	}
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled) && !can_submit) {
		if (m_upload_replay_handle.fetching())
			ImGui::SetTooltip("Submitting...");
		else if (!authed)
			ImGui::SetTooltip("Log in to submit times!");
		else
			ImGui::SetTooltip("Cannot submit slower record.");
	}

	ImGui::EndDisabled();
}

void edit::m_gui_menu(fsm* sm) {
	if (ImGui::ImageButtonWithText(resource::get().imtex("assets/gui/search.png"), "Search###Search")) {
		return sm->swap_state<states::search>();
	}
	ImGui::BeginDisabled();
	if (ImGui::ImageButtonWithText(resource::get().imtex("assets/gui/folder.png"), "Levels###Levels")) {
		//return sm->swap_state<states::search>();
	}
	ImGui::EndDisabled();

	m_quickplay_handle.poll();
	ImGui::BeginDisabled(m_quickplay_handle.fetching());
	if (ImGui::ImageButtonWithText(resource::get().imtex("assets/gui/dice.png"), "Random Level")) {
		m_quickplay_handle.reset(api::get().quickplay_level());
	}
	ImGui::EndDisabled();
	if (!m_quickplay_handle.fetching() && m_quickplay_handle.ready()) {
		auto res = m_quickplay_handle.get();
		if (!res.success) {
			ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetColorU32(sf::Color::Red));
			ImGui::TextWrapped("%s", m_quickplay_handle.get().error->c_str());
			ImGui::PopStyleColor();
		} else {
			sm->swap_state<states::edit>(*res.level);
		}
	}
}

void edit::m_gui_level_info(fsm* sm) {
	api::level md = m_level().get_metadata();
	ImGui::Text("-= Details (ID %d) =-", md.id);
	ImGui::TextWrapped("Title: %s", md.title.c_str());
	ImGui::TextWrapped("Author: %s", md.author.c_str());
	ImGui::TextWrapped("Downloads: %d", md.downloads);
	if (md.record) {
		ImGui::TextWrapped("World record: %.2fs by %s", md.record->time, md.record->user.c_str());
	}
	if (md.myRecord) {
		ImGui::TextWrapped("Your best: %.2fs", md.myRecord->time);
	}
	ImGui::TextWrapped("Likes: %d", md.likes);
	ImGui::TextWrapped("Dislikes: %d", md.dislikes);
	if (md.likes > 0 || md.dislikes > 0) {
		ImGui::TextWrapped("Ratio: %.2f", float(md.likes - md.dislikes) / float(md.likes + md.dislikes));
	}
	char date_fmt[100];
	tm* date_tm = std::localtime(&md.createdAt);
	std::strftime(date_fmt, 100, "%D %r", date_tm);
	ImGui::TextWrapped("Created: %s", date_fmt);
	date_tm = std::localtime(&md.updatedAt);
	std::strftime(date_fmt, 100, "%D %r", date_tm);
	ImGui::TextWrapped("Last Updated: %s", date_fmt);
	ImGui::Separator();
	ImGui::TextWrapped("Description: %s", md.description.c_str());
}

void edit::m_gui_controls(fsm* sm) {
	sf::Vector2f wsz(resource::get().window().getSize());
	ImTextureID icon  = !m_test_playing() ? resource::get().imtex("assets/gui/play.png") : resource::get().imtex("assets/gui/stop.png");
	std::string label = !m_test_playing() ? "Play" : "Edit";
	if (ImGui::ImageButtonWithText(icon, label.c_str())) {
		m_toggle_test_play();
	}
	if (ImGui::ImageButtonWithText(resource::get().imtex("assets/gui/export.png"), "Export")) {
		ImGui::OpenPopup("Export###Export");
	}
	ImGui::SameLine();
	if (ImGui::ImageButtonWithText(resource::get().imtex("assets/gui/import.png"), "Import")) {
		std::memset(m_import_buffer, 0, 8192);
		ImGui::OpenPopup("Import###Import");
	}
	if (ImGui::ImageButtonWithText(resource::get().imtex("assets/gui/erase.png"), "Clear")) {
		ImGui::OpenPopup("Clear###Confirm");
	}
	ImGui::BeginDisabled(!m_is_current_level_ours() || !m_level().valid() || !auth::get().authed());
	if (ImGui::ImageButtonWithText(resource::get().imtex("assets/gui/upload.png"), "Upload")) {
		ImGui::OpenPopup("Upload###Upload");
	}
	ImGui::EndDisabled();
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
		if (!auth::get().authed()) {
			ImGui::SetTooltip("You must be logged in to upload levels.");
		} else if (!m_level().valid()) {
			ImGui::SetTooltip("Cannot upload a level without a start & end point.");
		} else if (!m_is_current_level_ours()) {
			ImGui::SetTooltip("Cannot re-upload someone else's level. Clear first to make your own level for posting.");
		}
	}
	ImGui::SameLine();
	if (ImGui::ImageButtonWithText(resource::get().imtex("assets/gui/download.png"), "Download")) {
		ImGui::OpenPopup("Download###Download");
	}
	///////////////// DOWNLOAD LOGIC ////////////////////////
	bool dummy_open = true;
	ImGuiWindowFlags modal_flags = ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize;
	if (ImGui::BeginPopup("Download###Download")) {
		if (m_download_handle.ready() && !m_download_handle.get().success) {
			ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetColorU32(sf::Color::Red));
			ImGui::TextWrapped("%s", m_download_handle.get().error->c_str());
			ImGui::PopStyleColor();
		}
		ImGui::InputScalar("Level ID###DownloadId", ImGuiDataType_S32, &m_id);
		ImGui::BeginDisabled(m_download_handle.fetching());
		if (ImGui::ImageButtonWithText(resource::get().imtex("assets/gui/download.png"), "Download")) {
			if (!m_download_handle.fetching())
				m_download_handle.reset(api::get().download_level(m_id));
		}
		ImGui::EndDisabled();
		if (m_download_handle.ready()) {
			auto res = m_download_handle.get();
			if (res.success) {
				m_load_api_level(*res.level);
				ImGui::CloseCurrentPopup();
			}
		}

		ImGui::EndPopup();
	}
	///////////////// UPLOAD LOGIC ////////////////////////
	bool upload_modal_open = m_level().valid();
	if (ImGui::BeginPopupModal("Upload###Upload", &upload_modal_open, modal_flags)) {
		if (m_upload_handle.ready() && !m_upload_handle.get().success && m_upload_handle.get().error) {
			ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetColorU32(sf::Color::Red));
			ImGui::TextWrapped("%s", m_upload_handle.get().error->c_str());
			ImGui::PopStyleColor();
		}
		if (ImGui::InputText("Title", m_title_buffer, 50, ImGuiInputTextFlags_EnterReturnsTrue)) {
			ImGui::SetKeyboardFocusHere(0);
		}
		if (ImGui::InputText("Description", m_description_buffer, 256, ImGuiInputTextFlags_EnterReturnsTrue)) {
			if (!m_upload_handle.fetching())
				m_upload_handle.reset(api::get().upload_level(m_level(), m_title_buffer, m_description_buffer));
		}
		ImGui::BeginDisabled(m_upload_handle.fetching());
		const char* upload_label = m_upload_handle.fetching() ? "Uploading...###UploadForReal" : "Upload###UploadForReal";
		if (ImGui::ImageButtonWithText(resource::get().imtex("assets/gui/upload.png"), upload_label)) {
			if (!m_upload_handle.fetching())
				m_upload_handle.reset(api::get().upload_level(m_level(), m_title_buffer, m_description_buffer));
		}
		ImGui::EndDisabled();
		if (m_upload_handle.ready()) {
			auto res = m_upload_handle.get();
			if (res.success) {
				m_load_api_level(*res.level);
				ImGui::CloseCurrentPopup();
			} else if (res.code == 409) {
				ImGui::OpenPopup("Upload###ConfirmUpload");
			}
			if (ImGui::BeginPopupModal("Confirm###ConfirmUpload", nullptr, modal_flags)) {
				ImGui::TextWrapped("A level named %s already exists, do you want to overwrite it?", m_title_buffer);
				if (ImGui::ImageButtonWithText(resource::get().imtex("assets/gui/yes.png"), "Yes###OverrideYes")) {
					m_upload_handle.reset();
					m_upload_handle.reset(api::get().upload_level(m_level(), m_title_buffer, m_description_buffer, true));
					ImGui::CloseCurrentPopup();
				}
				ImGui::SameLine();
				if (ImGui::ImageButtonWithText(resource::get().imtex("assets/gui/no.png"), "No###OverrideNo")) {
					m_upload_handle.reset();
					ImGui::CloseCurrentPopup();
				}
				ImGui::EndPopup();
			}
		}
		ImGui::EndPopup();
	}
	///////////////// UPLOAD LOGIC END ////////////////////////
	ImGui::Separator();
	if (m_loaded_replay) {
		ImGui::Text("Replay: %.2f by %s%s", m_loaded_replay->get_time(), m_loaded_replay->get_user(), m_loaded_replay->alt() ? " (bb controls)" : "");
		if (ImGui::ImageButtonWithText(resource::get().imtex("assets/gui/erase.png"), "Unload###UL")) {
			m_loaded_replay.reset();
		}
		ImGui::SameLine();
	} else {
		ImGui::Text("No replay loaded");
	}
	if (ImGui::ImageButtonWithText(resource::get().imtex("assets/gui/folder.png"), "Load Replay File###LoadReplay")) {
		ImGuiFileDialog::Instance()->OpenDialog("ChooseReplayFile", "Choose Replay File", ".rpl", ".", 1, nullptr, ImGuiFileDialogFlags_Modal);
	}
	if (ImGuiFileDialog::Instance()->Display("ChooseReplayFile", ImGuiWindowFlags_NoResize, wsz * 0.5f)) {
		if (ImGuiFileDialog::Instance()->IsOk()) {
			auto path = ImGuiFileDialog::Instance()->GetFilePathName();
			replay rp;
			rp.load_from_file(path);
			m_loaded_replay = rp;
		}
		ImGuiFileDialog::Instance()->Close();
	}

	if (ImGui::BeginPopup("Clear###Confirm")) {
		ImGui::Text("Are you sure you want to erase the whole level?");
		if (ImGui::ImageButtonWithText(resource::get().imtex("assets/gui/yes.png"), "Yes")) {
			resource::get().play_sound("gameover");
			if (m_test_playing())
				m_toggle_test_play();
			m_clear_level();
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::ImageButtonWithText(resource::get().imtex("assets/gui/no.png"), "No")) {
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
	// import / export popups
	if (ImGui::BeginPopupModal("Export###Export")) {
		std::string saved = m_level().map().save();
		ImGui::BeginChildFrame(ImGui::GetID("###ExportText"), ImVec2(-1, 200));
		ImGui::TextWrapped("%s", saved.c_str());
		ImGui::EndChildFrame();
		ImGui::Text("Save this code or send it to a friend!");
		if (ImGui::ImageButtonWithText(resource::get().imtex("assets/gui/copy.png"), "Copy")) {
			ImGui::SetClipboardText(saved.c_str());
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::ImageButtonWithText(resource::get().imtex("assets/gui/back.png"), "Done")) {
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
	if (ImGui::BeginPopupModal("Import###Import")) {
		ImGui::InputText("Import Level Code", m_import_buffer, 8192);
		if (ImGui::ImageButtonWithText(resource::get().imtex("assets/gui/paste.png"), "Paste")) {
			std::strcpy(m_import_buffer, ImGui::GetClipboardText());
		}
		ImGui::SameLine();
		if (ImGui::ImageButtonWithText(resource::get().imtex("assets/gui/create.png"), "Load")) {
			m_clear_level();
			m_level().map().load(std::string(m_import_buffer));
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::ImageButtonWithText(resource::get().imtex("assets/gui/back.png"), "Cancel")) {
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
}

void edit::m_gui_block_picker(fsm* sm) {
	ImGuiWindowFlags flags = ImGuiWindowFlags_None;
	flags |= ImGuiWindowFlags_AlwaysAutoResize;

	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
	ImGui::BeginChildFrame(ImGui::GetID("BlocksPicker"), ImVec2(32 * 8, 32 * 9.5f), flags);
	for (tile::tile_type i = tile::begin; i <= tile::ice; i = (tile::tile_type)(int(i) + 1)) {
		ImGui::PushID((int)i);
		if (ImGui::EditorTileButton(m_tiles, i, m_level(), m_selected_tile == i)) {
			m_selected_tile = i;
		}
		if (ImGui::IsItemHovered()) {
			ImGui::SetTooltip("%s", tile::description(i).c_str());
		}
		ImGui::SameLine();
		ImGui::PopID();
	}
	ImGui::NewLine();
	for (tile::tile_type i = tile::black; i <= tile::ladder; i = (tile::tile_type)(int(i) + 1)) {
		ImGui::PushID((int)i);
		if (ImGui::EditorTileButton(m_tiles, i, m_level(), m_selected_tile == i)) {
			m_selected_tile = i;
		}
		if (ImGui::IsItemHovered()) {
			ImGui::SetTooltip("%s", tile::description(i).c_str());
		}
		ImGui::SameLine();
		ImGui::PopID();
	}
	ImGui::NewLine();
	for (tile::tile_type i = tile::stopper; i <= tile::erase; i = (tile::tile_type)(int(i) + 1)) {
		ImGui::PushID((int)i);
		if (ImGui::EditorTileButton(m_tiles, i, m_level(), m_selected_tile == i)) {
			m_selected_tile = i;
		}
		if (ImGui::IsItemHovered()) {
			ImGui::SetTooltip("%s", tile::description(i).c_str());
		}
		ImGui::SameLine();
		ImGui::PopID();
	}
	ImGui::NewLine();
	ImGui::TextWrapped("%s", tile::description(m_selected_tile).c_str());
	for (tile::tile_type i = tile::move_up; i <= tile::move_none; i = (tile::tile_type)(int(i) + 1)) {
		ImGui::PushID((int)i);
		if (ImGui::EditorTileButton(m_tiles, i, m_level(), m_selected_direction == i)) {
			m_selected_direction = i;
		}
		if (ImGui::IsItemHovered()) {
			ImGui::SetTooltip("%s", tile::description(i).c_str());
		}
		ImGui::SameLine();
		ImGui::PopID();
	}
	ImGui::PopStyleVar();
	ImGui::NewLine();
	ImGui::TextWrapped("%s", tile::description(m_selected_direction).c_str());
	// pencil options
	if (ImGui::EditorTileButton(m_tools, (tile::tile_type)PENCIL, m_level(), m_cursor_type == PENCIL, 32)) {
		m_cursor_type = PENCIL;
	}
	if (ImGui::IsItemHovered()) {
		ImGui::SetTooltip("%s", m_cursor_description(PENCIL));
	}
	ImGui::SameLine();
	if (ImGui::EditorTileButton(m_tools, (tile::tile_type)FLOOD, m_level(), m_cursor_type == FLOOD, 32)) {
		m_cursor_type = FLOOD;
	}
	if (ImGui::IsItemHovered()) {
		ImGui::SetTooltip("%s", m_cursor_description(FLOOD));
	}
	ImGui::SameLine();
	if (ImGui::EditorTileButton(m_tools, (tile::tile_type)STROKE, m_level(), m_cursor_type == STROKE, 32)) {
		m_cursor_type = STROKE;
	}
	if (ImGui::IsItemHovered()) {
		ImGui::SetTooltip("%s", m_cursor_description(STROKE));
	}
	ImGui::TextWrapped("%s", m_cursor_description(m_cursor_type));
	ImGui::BeginDisabled(m_undo_queue.empty());
	if (ImGui::ImageButtonWithText(resource::get().imtex("assets/gui/back.png"), "Undo")) {
		std::vector<tilemap::diff> diffs = m_undo_queue.back();
		m_undo_queue.pop_back();
		m_redo_queue.push_back(diffs);
		for (auto& diff : diffs) {
			m_level().map().undo(diff);
		}
	}
	ImGui::EndDisabled();
	ImGui::SameLine();
	ImGui::BeginDisabled(m_redo_queue.empty());
	if (ImGui::ImageButtonWithText(resource::get().imtex("assets/gui/forward.png"), "Redo")) {
		std::vector<tilemap::diff> diffs = m_redo_queue.back();
		m_redo_queue.pop_back();
		m_undo_queue.push_back(diffs);
		for (auto& diff : diffs) {
			m_level().map().redo(diff);
		}
	}
	ImGui::EndDisabled();

	ImGui::EndChildFrame();
}

void edit::m_toggle_test_play() {
	if (!m_test_playing()) {
		if (!m_level().valid()) {
			m_info_msg = "Cannot start without a valid start & end position";
			return;
		}
		m_level().map().set_editor_view(false);
		m_info_msg = "";
		m_test_play_world.reset(new world(m_level(), m_loaded_replay));
		resource::get().play_music("game_bg");
		m_update_transforms();
	} else {
		m_test_play_world.reset();
		m_upload_replay_handle.reset();
		m_level().map().set_editor_view(true);
		resource::get().play_music("menu_bg");
	}
}

bool edit::m_test_playing() const {
	return !!m_test_play_world;
}

const char* edit::m_cursor_description(edit::cursor_type c) const {
	switch (c) {
	case PENCIL:
		return "Place one tile at a time";
	case FLOOD:
		return "Flood fill an area with the same type of tile";
	case STROKE:
		return "Draw straight lines (hold SHIFT to draw X/Y parallel lines)";
	}
}

void edit::draw(sf::RenderTarget& t, sf::RenderStates s) const {
	s.transform *= getTransform();
	t.draw(m_bg, s);
	t.draw(m_map, s);
}
}
