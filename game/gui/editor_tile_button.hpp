#pragma once

#include <imgui-SFML.h>
#include <imgui.h>
#include <imgui_internal.h>

#include "../level.hpp"
#include "../tilemap.hpp"

namespace ImGui {

IMGUI_API bool EditorTileButton(
	sf::Texture& tiles,
	tile::tile_type type,
	const level& level,
	bool selected = false);

}
