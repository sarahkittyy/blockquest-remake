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
	  m_border(r.tex("assets/tiles.png"), 34, 34, 16),
	  m_tiles(r.tex("assets/tiles.png")) {
	m_level.setOrigin(m_level.map().total_size() / 2.f);
	m_level.setPosition((sf::Vector2f)r.window().getSize() * 0.5f);
	m_level.setScale(2.f, 2.f);
	m_level.map().set_editor_view(true);

	m_cursor.setOrigin(m_cursor.map().total_size() / 2.f);
	m_cursor.setPosition((sf::Vector2f)r.window().getSize() * 0.5f);
	m_cursor.setScale(2.f, 2.f);
	m_cursor.map().set_editor_view(true);

	m_border.setOrigin(m_border.total_size() / 2.f);
	m_border.setPosition((sf::Vector2f)r.window().getSize() * 0.5f);
	m_border.setScale(2.f, 2.f);
	m_border.set_editor_view(true);

	debug::get().setPosition(m_level.getPosition().x - m_level.getOrigin().x * 2.f, 32);
	debug::get().setScale(m_level.getScale());

	for (int i = 0; i < 34; ++i) {
		m_border.set(i, 0, tile::block);
		m_border.set(i, 33, tile::block);
		m_border.set(0, i, tile::block);
		m_border.set(33, i, tile::block);
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
		bool success = m_set_tile(mouse_tile, m_selected_tile, m_last_debug_msg);
	}

	if (m_test_playing()) {
		m_test_play_world->update(dt);
	}

	debug::get() << "Debug: " << m_last_debug_msg << "\n";
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

void edit::imdraw(fsm* sm) {
	// clang-format off
	ImGuiWindowFlags flags = ImGuiWindowFlags_None;
	flags |= ImGuiWindowFlags_NoResize;
	flags |= ImGuiWindowFlags_AlwaysAutoResize;

	// controls
	bool popup_open = false;
	ImGui::SetNextWindowPos(ImVec2(100, 100), ImGuiCond_FirstUseEver);
	ImGui::Begin("Controls", nullptr); {
		ImTextureID icon = !m_test_playing() ? r().imtex("assets/gui/play.png") : r().imtex("assets/gui/stop.png");
		std::string label = !m_test_playing() ? "Test Play" : "Edit";
		if (ImGui::ImageButtonWithText(icon, label.c_str())) {
			m_toggle_test_play();
		}
		if (ImGui::ImageButtonWithText(r().imtex("assets/gui/export.png"), "Export")) {
			ImGui::OpenPopup("Export###Export");
		}
		ImGui::SameLine();
		if (ImGui::ImageButtonWithText(r().imtex("assets/gui/import.png"), "Import")) {
			std::memset(import_buffer, 0, 8192);
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
			ImGui::InputText("Import Level Code", import_buffer, 8192);
			if (ImGui::ImageButtonWithText(r().imtex("assets/gui/paste.png"), "Paste")) {
				std::strcpy(import_buffer, ImGui::GetClipboardText());
			}
			ImGui::SameLine();
			if (ImGui::ImageButtonWithText(r().imtex("assets/gui/create.png"), "Load")) {
				m_level.map().load(std::string(import_buffer));
				ImGui::CloseCurrentPopup();
			}
			ImGui::SameLine();
			if (ImGui::ImageButtonWithText(r().imtex("assets/gui/back.png"), "Cancel")) {
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}
	} ImGui::End();

	// block picker
	ImGui::SetNextWindowPos(ImVec2(100, 500), ImGuiCond_FirstUseEver);
	ImGui::Begin("Blocks", nullptr, flags); {
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
	ImGui::BeginChildFrame(ImGui::GetID("BlocksPicker"), ImVec2(32 * 8, 32 * 8), flags);
		for(tile::tile_type i = tile::begin; i <= tile::ice; i = (tile::tile_type)(int(i) + 1)) {
			ImGui::PushID((int)i);
			if (ImGui::EditorTileButton(m_tiles, i, m_level, m_selected_tile == i)) {
				m_selected_tile = i;
			}
			ImGui::SameLine();
			ImGui::PopID();
		}
		ImGui::NewLine();
		for(tile::tile_type i = tile::black; i <= tile::ladder; i = (tile::tile_type)(int(i) + 1)) {
			ImGui::PushID((int)i);
			if (ImGui::EditorTileButton(m_tiles, i, m_level, m_selected_tile == i)) {
				m_selected_tile = i;
			}
			ImGui::SameLine();
			ImGui::PopID();
		}
		ImGui::NewLine();
		for(tile::tile_type i = tile::stopper; i <= tile::erase; i = (tile::tile_type)(int(i) + 1)) {
			ImGui::PushID((int)i);
			if (ImGui::EditorTileButton(m_tiles, i, m_level, m_selected_tile == i)) {
				m_selected_tile = i;
			}
			ImGui::SameLine();
			ImGui::PopID();
		}
		ImGui::NewLine();
		for(tile::tile_type i = tile::move_up; i <= tile::move_none; i = (tile::tile_type)(int(i) + 1)) {
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
	ImGui::EndChildFrame();
	} ImGui::End();

	// clang-format on
}

void edit::m_toggle_test_play() {
	if (!m_test_playing()) {
		if (!m_level.valid()) {
			m_last_debug_msg = "Cannot start without a valid start & end position";
			return;
		}
		m_level.map().set_editor_view(false);
		m_test_play_world.reset(new world(r(), m_level));
		m_test_play_world->setOrigin(m_level.map().total_size() / 2.f);
		m_test_play_world->setPosition((sf::Vector2f)r().window().getSize() * 0.5f);
		m_test_play_world->setScale(2.f, 2.f);
	} else {
		m_test_play_world.reset();
		m_level.map().set_editor_view(true);
	}
}

bool edit::m_test_playing() const {
	return !!m_test_play_world;
}

void edit::draw(sf::RenderTarget& t, sf::RenderStates s) const {
	s.transform *= getTransform();
	t.draw(m_border, s);
	if (!m_test_playing()) {
		t.draw(m_level, s);
		t.draw(m_cursor, s);
	} else {
		t.draw(*m_test_play_world, s);
	}
}
}
