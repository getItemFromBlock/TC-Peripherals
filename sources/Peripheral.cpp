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
	ImGui::Text("Current offset: %#010x", offset);
	ImGui::InputText("offset", &offsetAsText, ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_CharsNoBlank);
	ImGui::SameLine();
	if (ImGui::SmallButton("Set offset"))
	{
		offset = std::atoi(offsetAsText.c_str());
		return true;
	}

	return false;
}