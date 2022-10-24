#include "image_text_button.hpp"

namespace ImGui {

bool ImageButtonWithText(ImTextureID texId, const char* label, const ImVec2& imageSize, const ImVec2& uv0, const ImVec2& uv1, int frame_padding, const ImVec4& bg_col, const ImVec4& tint_col) {
	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return false;

	sf::Vector2f size = imageSize;
	if (size.x <= 0 && size.y <= 0) {
		size.x = size.y = ImGui::GetTextLineHeightWithSpacing();
	} else {
		if (size.x <= 0)
			size.x = size.y;
		else if (size.y <= 0)
			size.y = size.x;
		size *= window->FontWindowScale * ImGui::GetIO().FontGlobalScale;
	}

	ImGuiContext& g			= *GImGui;
	const ImGuiStyle& style = g.Style;

	const ImGuiID id			= window->GetID(label);
	const sf::Vector2f textSize = ImGui::CalcTextSize(label, NULL, true);
	const bool hasText			= textSize.x > 0;

	const float innerSpacing   = hasText ? ((frame_padding >= 0) ? (float)frame_padding : (style.ItemInnerSpacing.x)) : 0.f;
	const sf::Vector2f padding = (frame_padding >= 0) ? sf::Vector2f((float)frame_padding, (float)frame_padding) : sf::Vector2f(style.FramePadding);
	const sf::Vector2f totalSizeWithoutPadding(size.x + innerSpacing + textSize.x, size.y > textSize.y ? size.y : textSize.y);
	sf::Vector2f cursorPos = window->DC.CursorPos;
	ImRect bb(cursorPos, cursorPos + totalSizeWithoutPadding + padding * 2.f);
	sf::Vector2f start(0, 0);
	start = cursorPos + padding;
	if (size.y < textSize.y) start.y += (textSize.y - size.y) * .5f;
	const ImRect image_bb(start, start + size);
	start = cursorPos + padding;
	start.x += size.x + innerSpacing;
	if (size.y > textSize.y) start.y += (size.y - textSize.y) * .5f;
	ItemSize(bb);
	if (!ItemAdd(bb, id))
		return false;

	bool hovered = false, held = false;
	bool pressed = ButtonBehavior(bb, id, &hovered, &held);

	// Render
	const ImU32 col = GetColorU32((hovered && held)
									  ? ImGuiCol_ButtonActive
								  : hovered
									  ? ImGuiCol_ButtonHovered
									  : ImGuiCol_Button);
	RenderFrame(bb.Min, bb.Max, col, true, ImClamp((float)ImMin(padding.x, padding.y), 0.0f, style.FrameRounding));
	if (bg_col.w > 0.0f)
		window->DrawList->AddRectFilled(image_bb.Min, image_bb.Max, GetColorU32(bg_col));

	window->DrawList->AddImage(texId, image_bb.Min, image_bb.Max, uv0, uv1, GetColorU32(tint_col));

	if (textSize.x > 0) ImGui::RenderText(start, label);
	return pressed;
}

}
