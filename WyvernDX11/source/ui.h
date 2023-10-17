#pragma once

struct ldiViewportSurfaceResult {
	bool isHovered;
	vec2 worldPos;
};

ldiViewportSurfaceResult uiViewportSurface2D(const char* SurfaceId, float* Scale, vec2* Offset) {
	ldiViewportSurfaceResult result = {};

	ImVec2 viewSize = ImGui::GetContentRegionAvail();
	//ImVec2 startPos = ImGui::GetCursorPos();
	ImVec2 screenStartPos = ImGui::GetCursorScreenPos();

	// This will catch our interactions.
	ImGui::InvisibleButton(SurfaceId, viewSize, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight | ImGuiButtonFlags_MouseButtonMiddle);
	const bool isHovered = ImGui::IsItemHovered(); // Hovered
	const bool isActive = ImGui::IsItemActive();   // Held
	ImVec2 mousePos = ImGui::GetIO().MousePos;
	const ImVec2 mouseCanvasPos(mousePos.x - screenStartPos.x, mousePos.y - screenStartPos.y);

	const bool isMouseRight = ImGui::GetIO().MouseDown[1];
	const bool isMouseLeft = ImGui::GetIO().MouseDown[0];

	result.isHovered = isHovered;

	// Convert canvas pos to world pos.
	vec2 worldPos;
	worldPos.x = mouseCanvasPos.x;
	worldPos.y = mouseCanvasPos.y;
	worldPos *= (1.0 / *Scale);
	worldPos -= *Offset;

	result.worldPos = worldPos;

	if (isHovered) {
		float wheel = ImGui::GetIO().MouseWheel;

		if (wheel) {
			*Scale += wheel * 0.2f * *Scale;

			if (*Scale < 0.01) {
				*Scale = 0.01;
			}

			vec2 newWorldPos;
			newWorldPos.x = mouseCanvasPos.x;
			newWorldPos.y = mouseCanvasPos.y;
			newWorldPos *= (1.0 / *Scale);
			newWorldPos -= *Offset;

			vec2 deltaWorldPos = newWorldPos - worldPos;

			Offset->x += deltaWorldPos.x;
			Offset->y += deltaWorldPos.y;
		}
	}

	if (isActive && ImGui::IsMouseDragging(ImGuiMouseButton_Right)) {
		vec2 mouseDelta = vec2(ImGui::GetIO().MouseDelta.x, ImGui::GetIO().MouseDelta.y);
		mouseDelta *= (1.0 / *Scale);

		*Offset += vec2(mouseDelta.x, mouseDelta.y);
	}

	return result;
}