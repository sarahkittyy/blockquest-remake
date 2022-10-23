#include "editor_tile_button.hpp"

#include <SFML/Graphics.hpp>

#include "../debug.hpp"
#include "imgui.h"

namespace ImGui {

bool EditorTileButton(
	sf::Texture& tiles,
	tile::tile_type type,
	const level& level,
	bool selected,
	int tile_size) {

	ImGuiWindow* window = ImGui::GetCurrentWindow();
	// size of the tiles texture in tiles
	sf::Vector2i tex_sz(
		tiles.getSize().x / tile_size,
		tiles.getSize().y / tile_size);
	// size of a tile in uv coords (0-1)
	sf::Vector2f tsz(
		1.f / tex_sz.x,
		1.f / tex_sz.y);
	int x	   = type % tex_sz.x;
	int y	   = type / tex_sz.x;
	ImVec2 uv0 = ImVec2(x * tsz.x, y * tsz.y);

	sf::Vector2f image_size(32, 32);

	static const ImVec4 hovered_tint(0.8, 0.8, 1, 1);
	static const ImVec4 selected_tint(1, 1, 1, 1);
	static const ImVec4 hovered_border(0, 0, 0, 0);
	static const ImVec4 selected_border(0.2, 0.2, 1, 1);

	sf::Vector2f frame_padding = ImGui::GetStyle().FramePadding;

	sf::Vector2f cpos = window->DC.CursorPos;
	ImRect bb(cpos, cpos + image_size);
	ImRect bb_bigger = bb;
	bb_bigger.Expand(2);
	ImGuiID id = window->GetIDFromRectangle(bb);

	ImGui::ItemSize(bb);
	if (!ImGui::ItemAdd(bb, id))
		return false;

	bool hovered = false;
	bool held	 = false;
	bool pressed = ImGui::ButtonBehavior(
		bb_bigger,
		id,
		&hovered,
		&held,
		ImGuiButtonFlags_MouseButtonLeft);
	selected |= held || pressed;
	if (selected) {
		window->DrawList->AddRect(bb_bigger.Min, bb_bigger.Max, ImGui::GetColorU32(selected_border));
	} else if (hovered) {
		window->DrawList->AddRect(bb_bigger.Min, bb_bigger.Max, ImGui::GetColorU32(hovered_border));
	}
	window->DrawList->AddImage(
		reinterpret_cast<ImTextureID>(tiles.getNativeHandle()),
		bb.Min,
		bb.Max,
		uv0,
		ImVec2(uv0.x + tsz.x, uv0.y + tsz.y),
		selected
			? ImGui::GetColorU32(selected_tint)
		: hovered
			? ImGui::GetColorU32(hovered_tint)
			: 0xFFFFFFFF);

	return pressed;
}

}
