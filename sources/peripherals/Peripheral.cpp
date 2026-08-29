#include <peripherals/Peripheral.hpp>
#include <imgui/imgui.h>
#include <imgui/imgui_stdlib.h>

Peripheral::~Peripheral()
{
}

void Peripheral::SetAddress(u64 value)
{
	address = value + offset;
}

bool Peripheral::DrawGuiBase(const char *title, bool &open, bool &visible)
{
	open = ImGui::CollapsingHeader(title, &visible, ImGuiTreeNodeFlags_DefaultOpen);
	if (open)
	{
		ImGui::Indent(20);
		ImGui::Text("Current offset: %#010x", offset);
		ImGui::InputText("offset", &offsetAsText, ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_CharsNoBlank);
		ImGui::SameLine();
		if (ImGui::SmallButton("Set offset"))
		{
			offset = std::atoi(offsetAsText.c_str());
			return true;
		}
	}

	return false;
}

bool Peripheral::DrawGuiEnd()
{
	ImGui::Unindent(20);
	return false;
}