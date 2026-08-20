#include <Peripheral.hpp>
#include <imgui/imgui.h>
#include <imgui/imgui_stdlib.h>

Peripheral::~Peripheral()
{
}

void Peripheral::SetAddress(u64 value)
{
	address = value + offset;
}

bool Peripheral::DrawGuiBase(const char *title)
{
	ImGui::BeginChild(title);
	ImGui::Text("%s", "Current offset:");
	ImGui::SameLine();
	bool res = ImGui::InputText("offset", &text, ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_CharsNoBlank);

	return false;
}