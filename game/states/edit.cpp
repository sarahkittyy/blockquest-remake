#include "edit.hpp"

#include "imgui-SFML.h"
#include "imgui.h"
#include "imgui_internal.h"

#include <cstring>
#include <stdexcept>

#include "../debug.hpp"
#include "../gui/editor_tile_button.hpp"
#include "../gui/image_text_button.hpp"

namespace states {

edit::edit(resource& r)
	: state(r),
	  m_level(r),
	  m_cursor(r),
	  m_border(r.tex("assets/tiles.png"), 34, 33, 16),
	  m_rules_gifs({ std::make_pair(ImGui::Gif(r.tex("assets/gifs/run.png"), 33, { 240, 240 }, 20),
									"Use left & right to run"),
					 std::make_pair(ImGui::Gif(r.tex("assets/gifs/jump.png"), 19, { 240, 240 }, 20),
									"Space to jump"),
					 std::make_pair(ImGui::Gif(r.tex("assets/gifs/dash.png"), 19, { 240, 240 }, 20),
									"Down to dash"),
					 std::make_pair(ImGui::Gif(r.tex("assets/gifs/wallkick.png"), 37, { 240, 240 }, 20),
									"Left + Right to wallkick"),
					 std::make_pair(ImGui::Gif(r.tex("assets/gifs/climb.png"), 25, { 240, 240 }, 20),
									"Up & Down to climb") }),
	  m_level_scale(2.f),
	  m_cursor_type(PENCIL),
	  m_tiles(r.tex("assets/tiles.png")),
	  m_tools(r.tex("assets/gui/tools.png")),
	  m_stroke_start(-1, -1),
	  m_stroke_active(false),
	  m_stroke_map(r.tex("assets/tiles.png"), 32, 32, 16) {

	sf::Vector2f win_sz(r.window().getSize());

	m_border.setOrigin(m_border.total_size().x / 2.f, m_border.total_size().y);
	m_stroke_map.setOrigin(m_stroke_map.total_size().x / 2.f, m_stroke_map.total_size().y);
	m_level.setOrigin(m_level.map().total_size().x / 2.f, m_level.map().total_size().y);
	m_cursor.setOrigin(m_cursor.map().total_size().x / 2.f, m_cursor.map().total_size().y);

	m_border.setScale(m_level_scale, m_level_scale);
	m_stroke_map.setScale(m_level_scale, m_level_scale);
	m_level.setScale(m_level_scale, m_level_scale);
	m_cursor.setScale(m_level_scale, m_level_scale);

	m_border.setPosition(win_sz.x / 2.f, win_sz.y);
	m_stroke_map.setPosition(win_sz.x / 2.f, win_sz.y);
	m_level.setPosition(win_sz.x / 2.f, win_sz.y);
	m_cursor.setPosition(win_sz.x / 2.f, win_sz.y);

	m_level.map().set_editor_view(true);
	m_cursor.map().set_editor_view(true);
	m_border.set_editor_view(true);
	m_stroke_map.set_editor_view(true);

	debug::get().setPosition(m_level.getPosition() - m_level.getOrigin() * 2.f);
	debug::get().setScale(m_level.getScale());

	for (int i = 0; i < 33; ++i) {
		m_border.set(0, i, tile::block);
		m_border.set(33, i, tile::block);
		m_border.set(i, 0, tile::block);
	}

	m_update_mouse_tile();
}

edit::~edit() {
	debug::get().setScale(1.f, 1.f);
	debug::get().setPosition(0, 0);
}

void edit::update(fsm* sm, sf::Time dt) {
	sf::Vector2i mouse_tile = m_update_mouse_tile();

	// if we're not on gui, and on the map, and ready to place a tile
	if (!ImGui::GetIO().WantCaptureMouse &&
		sf::Mouse::isButtonPressed(sf::Mouse::Left) &&
		!m_test_playing() &&
		m_level.map().in_bounds(mouse_tile)) {
		// set the tile
		switch (m_cursor_type) {
		case PENCIL:
			m_set_tile(mouse_tile, m_selected_tile, m_last_debug_msg);
			break;
		case FLOOD:
			m_flood_fill(mouse_tile, m_selected_tile, m_level.map().get(mouse_tile.x, mouse_tile.y).type, m_last_debug_msg);
			break;
		case STROKE:
			if (m_selected_tile == tile::begin || m_selected_tile == tile::end)
				m_set_tile(mouse_tile, m_selected_tile, m_last_debug_msg);
			else
				m_stroke_fill(mouse_tile, m_selected_tile, m_last_debug_msg);
			break;
		}
	} else {
		// if we're done stroking, render the line
		if (m_stroke_active) {
			m_stroke_active = false;
			m_stroke_map.layer_over(m_level.map(), true);
			m_stroke_map.clear();
		}
	}

	if (m_test_playing()) {
		m_test_play_world->update(dt);
	}
}

bool edit::m_stroke_fill(sf::Vector2i pos, tile::tile_type type, std::string& error) {
	// exclude certain types
	switch (type) {
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
		return false;
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
	m_stroke_map.set_line(m_stroke_start, pos, type);
	return true;
}

bool edit::m_set_tile(sf::Vector2i pos, tile::tile_type type, std::string& error) {
	try {
		tilemap& m	= m_level.map();
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
			m.set(x, y, type);
			break;
		case tile::empty:
		case tile::erase:
			m.clear(x, y);
			break;
		case tile::move_up:
		case tile::move_down:
		case tile::move_left:
		case tile::move_right:
			if (m.get(x, y).movable())
				m.set(x, y, tile(m.get(x, y).type, { .moving = int(type) - 13 }));
			break;
		case tile::move_none:
			m.set(x, y, tile(m.get(x, y).type, { .moving = 0 }));
			break;
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
			m.set(x, y, type);
			break;
		default:
			return false;
		}
		return true;
	} catch (const std::runtime_error& e) {
		error = e.what();
		return false;
	}
}

bool edit::m_flood_fill(sf::Vector2i pos, tile::tile_type type, tile::tile_type replacing, std::string& error) {
	try {
		tilemap& m	= m_level.map();
		const int x = pos.x;
		const int y = pos.y;
		tile t		= m.get(x, y);
		if (x < 0 || x >= m.size().x || y < 0 || y >= m.size().y) return true;
		if (t != replacing) return true;
		if (t == type) return true;
		if (type == replacing) return true;
		if (t == tile::empty && type == tile::erase) return true;
		switch (type) {
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
			return true;
		default:
			break;
		}

		if (m_set_tile(pos, type, error)) {
			m_flood_fill({ x - 1, y }, type, replacing, error);
			m_flood_fill({ x + 1, y }, type, replacing, error);
			m_flood_fill({ x, y - 1 }, type, replacing, error);
			m_flood_fill({ x, y + 1 }, type, replacing, error);
		}
	} catch (const std::runtime_error& e) {
		error = e.what();
		return false;
	}
	return true;
}

sf::Vector2i edit::m_update_mouse_tile() {
	const sf::Vector2i mouse_tile = m_level.mouse_tile();
	debug::get() << mouse_tile << "\n";
	if (mouse_tile != m_old_mouse_tile) {
		m_cursor.map().clear(m_old_mouse_tile.x, m_old_mouse_tile.y);
		m_old_mouse_tile = mouse_tile;
		if (m_cursor.map().in_bounds(mouse_tile)) {
			m_cursor.map().set(mouse_tile.x, mouse_tile.y, tile::cursor);
		}
	}
	return mouse_tile;
}

void edit::m_menu_bar(fsm* sm) {
	// rules
	if (ImGui::MenuItem("Rules")) {
		ImGui::OpenPopup("Rules###Rules");
	}
	if (ImGui::BeginPopup("Rules###Rules")) {
		ImGui::BeginTable("###Rules", 2);
		ImGui::TableNextColumn();
		ImRect uvs;

		uvs = m_level.map().calc_uvs(tile::begin);
		ImGui::Text("Start here");
		ImGui::TableNextColumn();
		ImGui::Image(
			reinterpret_cast<ImTextureID>(m_tiles.getNativeHandle()),
			ImVec2(64, 64),
			uvs.Min, uvs.Max);
		ImGui::TableNextRow();

		uvs = m_level.map().calc_uvs(tile::end);
		ImGui::TableNextColumn();
		ImGui::Text("Try to reach the goal");
		ImGui::TableNextColumn();
		ImGui::Image(
			reinterpret_cast<ImTextureID>(m_tiles.getNativeHandle()),
			ImVec2(64, 64),
			uvs.Min, uvs.Max);
		ImGui::TableNextRow();

		uvs = m_level.map().calc_uvs(tile::spike);
		ImGui::TableNextColumn();
		ImGui::Text("Avoid spikes");
		ImGui::TableNextColumn();
		ImGui::Image(
			reinterpret_cast<ImTextureID>(m_tiles.getNativeHandle()),
			ImVec2(64, 64),
			uvs.Min, uvs.Max);
		ImGui::TableNextRow();

		uvs = m_level.map().calc_uvs(tile::gravity);
		ImGui::TableNextColumn();
		ImGui::Text("Flip gravity");
		ImGui::TableNextColumn();
		ImGui::Image(
			reinterpret_cast<ImTextureID>(m_tiles.getNativeHandle()),
			ImVec2(64, 64),
			uvs.Min, uvs.Max);
		ImGui::TableNextRow();

		uvs = m_level.map().calc_uvs(tile::ladder);
		ImGui::TableNextColumn();
		ImGui::Text("Climb up ladders");
		ImGui::TableNextColumn();
		ImGui::Image(
			reinterpret_cast<ImTextureID>(m_tiles.getNativeHandle()),
			ImVec2(64, 64),
			uvs.Min, uvs.Max);
		ImGui::TableNextRow();

		uvs = m_level.map().calc_uvs(tile::ice);
		ImGui::TableNextColumn();
		ImGui::Text("Watch your step");
		ImGui::TableNextColumn();
		ImGui::Image(
			reinterpret_cast<ImTextureID>(m_tiles.getNativeHandle()),
			ImVec2(64, 64),
			uvs.Min, uvs.Max);
		ImGui::TableNextRow();

		ImGui::EndTable();
		ImGui::EndPopup();
	}
	// controls
	if (ImGui::MenuItem("Controls")) {
		ImGui::OpenPopup("Controls###Controls");
	}
	if (ImGui::BeginPopup("Controls###Controls")) {
		ImGui::BeginTable("###Gifs", 5);
		for (auto& [gif, desc] : m_rules_gifs) {
			ImGui::TableNextColumn();
			gif.update();
			gif.draw({ 240, 240 });
			ImGui::Text("%s", desc);
		}
		ImGui::EndTable();
		ImGui::EndPopup();
	}
	// debug msg
	if (m_last_debug_msg.length() != 0) {
		ImGui::TextColored(sf::Color(255, 120, 120, 255), "[!] %s", m_last_debug_msg.c_str());
		if (ImGui::MenuItem("OK")) {
			m_last_debug_msg = "";
		}
	}
}

void edit::m_controls(fsm* sm) {
	ImTextureID icon  = !m_test_playing() ? r().imtex("assets/gui/play.png") : r().imtex("assets/gui/stop.png");
	std::string label = !m_test_playing() ? "Test Play" : "Edit";
	if (ImGui::ImageButtonWithText(icon, label.c_str())) {
		m_toggle_test_play();
	}
	if (ImGui::ImageButtonWithText(r().imtex("assets/gui/export.png"), "Export")) {
		ImGui::OpenPopup("Export###Export");
	}
	ImGui::SameLine();
	if (ImGui::ImageButtonWithText(r().imtex("assets/gui/import.png"), "Import")) {
		std::memset(m_import_buffer, 0, 8192);
		ImGui::OpenPopup("Import###Import");
	}
	if (ImGui::ImageButtonWithText(r().imtex("assets/gui/erase.png"), "Clear")) {
		ImGui::OpenPopup("Clear###Confirm");
	}
	if (ImGui::BeginPopup("Clear###Confirm")) {
		ImGui::Text("Are you sure you want to erase the whole level?");
		if (ImGui::ImageButtonWithText(r().imtex("assets/gui/yes.png"), "Yes")) {
			r().play_sound("gameover");
			m_level.map().clear();
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::ImageButtonWithText(r().imtex("assets/gui/no.png"), "No")) {
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
	// import / export popups
	if (ImGui::BeginPopupModal("Export###Export")) {
		std::string saved = m_level.map().save();
		ImGui::BeginChildFrame(ImGui::GetID("###ExportText"), ImVec2(-1, 200));
		ImGui::TextWrapped("%s", saved.c_str());
		ImGui::EndChildFrame();
		ImGui::Text("Save this code or send it to a friend!");
		if (ImGui::ImageButtonWithText(r().imtex("assets/gui/copy.png"), "Copy")) {
			ImGui::SetClipboardText(saved.c_str());
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::ImageButtonWithText(r().imtex("assets/gui/back.png"), "Done")) {
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
	if (ImGui::BeginPopupModal("Import###Import")) {
		ImGui::InputText("Import Level Code", m_import_buffer, 8192);
		if (ImGui::ImageButtonWithText(r().imtex("assets/gui/paste.png"), "Paste")) {
			std::strcpy(m_import_buffer, ImGui::GetClipboardText());
		}
		ImGui::SameLine();
		if (ImGui::ImageButtonWithText(r().imtex("assets/gui/create.png"), "Load")) {
			m_level.map().load(std::string(m_import_buffer));
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::ImageButtonWithText(r().imtex("assets/gui/back.png"), "Cancel")) {
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
}

void edit::m_block_picker(fsm* sm) {
	ImGuiWindowFlags flags = ImGuiWindowFlags_None;
	flags |= ImGuiWindowFlags_NoResize;
	flags |= ImGuiWindowFlags_AlwaysAutoResize;

	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
	ImGui::BeginChildFrame(ImGui::GetID("BlocksPicker"), ImVec2(32 * 8, 32 * 8), flags);
	for (tile::tile_type i = tile::begin; i <= tile::ice; i = (tile::tile_type)(int(i) + 1)) {
		ImGui::PushID((int)i);
		if (ImGui::EditorTileButton(m_tiles, i, m_level, m_selected_tile == i)) {
			m_selected_tile = i;
		}
		ImGui::SameLine();
		ImGui::PopID();
	}
	ImGui::NewLine();
	for (tile::tile_type i = tile::black; i <= tile::ladder; i = (tile::tile_type)(int(i) + 1)) {
		ImGui::PushID((int)i);
		if (ImGui::EditorTileButton(m_tiles, i, m_level, m_selected_tile == i)) {
			m_selected_tile = i;
		}
		ImGui::SameLine();
		ImGui::PopID();
	}
	ImGui::NewLine();
	for (tile::tile_type i = tile::stopper; i <= tile::erase; i = (tile::tile_type)(int(i) + 1)) {
		ImGui::PushID((int)i);
		if (ImGui::EditorTileButton(m_tiles, i, m_level, m_selected_tile == i)) {
			m_selected_tile = i;
		}
		ImGui::SameLine();
		ImGui::PopID();
	}
	ImGui::NewLine();
	for (tile::tile_type i = tile::move_up; i <= tile::move_none; i = (tile::tile_type)(int(i) + 1)) {
		ImGui::PushID((int)i);
		if (ImGui::EditorTileButton(m_tiles, i, m_level, m_selected_tile == i)) {
			m_selected_tile = i;
		}
		ImGui::SameLine();
		ImGui::PopID();
	}
	ImGui::PopStyleVar();
	ImGui::NewLine();
	ImGui::TextWrapped("%s", tile::description(m_selected_tile).c_str());
	// pencil options
	if (ImGui::EditorTileButton(m_tools, (tile::tile_type)PENCIL, m_level, m_cursor_type == PENCIL, 32)) {
		m_cursor_type = PENCIL;
	}
	ImGui::SameLine();
	if (ImGui::EditorTileButton(m_tools, (tile::tile_type)FLOOD, m_level, m_cursor_type == FLOOD, 32)) {
		m_cursor_type = FLOOD;
	}
	ImGui::SameLine();
	if (ImGui::EditorTileButton(m_tools, (tile::tile_type)STROKE, m_level, m_cursor_type == STROKE, 32)) {
		m_cursor_type = STROKE;
	}
	ImGui::TextWrapped("%s", m_selected_cursor_description());

	ImGui::EndChildFrame();
}

void edit::imdraw(fsm* sm) {
	// clang-format off
	ImGuiWindowFlags flags = ImGuiWindowFlags_None;
	flags |= ImGuiWindowFlags_NoResize;
	flags |= ImGuiWindowFlags_AlwaysAutoResize;

	// menu bar
	ImGui::BeginMainMenuBar();
	m_menu_bar(sm);
	ImGui::EndMainMenuBar();

	// controls
	ImGui::SetNextWindowPos(ImVec2(100, 100), ImGuiCond_FirstUseEver);
	ImGui::Begin("Controls", nullptr);
	m_controls(sm);
	ImGui::End();

	// block picker
	ImGui::SetNextWindowPos(ImVec2(100, 500), ImGuiCond_FirstUseEver);
	ImGui::Begin("Blocks", nullptr, flags);
	m_block_picker(sm);
	ImGui::End();
}

void edit::m_toggle_test_play() {
	if (!m_test_playing()) {
		if (!m_level.valid()) {
			m_last_debug_msg = "Cannot start without a valid start & end position";
			return;
		}
		m_level.map().set_editor_view(false);
		m_last_debug_msg = "";
		m_test_play_world.reset(new world(r(), m_level));
		m_test_play_world->setOrigin(m_level.map().total_size().x / 2.f, m_level.map().total_size().y);
		m_test_play_world->setPosition(r().window().getSize().x / 2.f, r().window().getSize().y);
		m_test_play_world->setScale(m_level_scale, m_level_scale);
	} else {
		m_test_play_world.reset();
		m_level.map().set_editor_view(true);
	}
}

bool edit::m_test_playing() const {
	return !!m_test_play_world;
}

const char* edit::m_selected_cursor_description() const {
	switch (m_cursor_type) {
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
	t.draw(m_border, s);
	if (!m_test_playing()) {
		t.draw(m_level, s);
		t.draw(m_stroke_map, s);
		t.draw(m_cursor, s);
	} else {
		t.draw(*m_test_play_world, s);
	}
}
}
