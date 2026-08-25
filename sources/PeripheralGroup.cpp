#include <PeripheralGroup.hpp>
#include <thread>
#include <GameLinker.hpp>
#include <SDL3/SDL.h>
#include <imgui/imgui.h>
#include <imgui/imgui_stdlib.h>

PeripheralGroup::~PeripheralGroup()
{
	for (size_t i = 0; i < peripherals.size(); i++)
		delete peripherals[i];

	peripherals.clear();
}

void PeripheralGroup::SearchForPattern(const std::vector<u8>& pattern, u64 start)
{
	tryAutoBind = true;
	bindAttempts = 0;
	startAddress = start;
	boundAddress = 0;
	targetPattern = pattern;
	UpdatePatternText();
}

void PeripheralGroup::SetPattern(const std::vector<u8>& pattern)
{
	targetPattern = pattern;
	UpdatePatternText();
}

void PeripheralGroup::BindAt(u64 targetAddress)
{
	tryAutoBind = false;
	bindAttempts = 0;
	if (GameLinker::IsConnected() || GameLinker::ConnectToTC())
		boundAddress = targetAddress;
}

void PeripheralGroup::Update()
{
	if (tryAutoBind && bindAttempts < 8 && targetPattern.size() > 0)
	{
		if (bindAttempts == 0)
		{
			if (!GameLinker::IsConnected())
				GameLinker::ConnectToTC();
			boundAddress = GameLinker::FindInProgram(targetPattern, startAddress > boundAddress ? startAddress : boundAddress);
			if (boundAddress != 0)
			{
				bindAttempts++;
			}
			else
			{
				SDL_LogWarn(0, "Could not find pattern in program!");
				bindAttempts = 0;
				tryAutoBind = false;
			}
		}
		else
		{
			u64 size = targetPattern.size();
			u8 *buf = new u8[size];
			bool success = buf != 0 && GameLinker::ReadMemory(boundAddress, buf, targetPattern.size());
			for (u64 i = 0; i < size && success; i++)
				success &= (buf[i] == targetPattern[i] || buf[i] == targetPattern[size - 1 - i]);
			delete[] buf;
			if (success)
			{
				bindAttempts++;
				if (bindAttempts == 8)
					UpdatePeripheralsAddress();
				else
					std::this_thread::sleep_for(std::chrono::milliseconds(50));
			}
			else
			{
				bindAttempts = 0;
				startAddress = boundAddress;
			}
		}
	}
}

void PeripheralGroup::BackgroundUpdate()
{
	if (!GameLinker::IsConnected() || !IsBound())
		return;
	for (size_t i = 0; i < peripherals.size(); i++)
		peripherals[i]->Update();
}

bool PeripheralGroup::DrawGui()
{
	bool open = true;
	ImGui::Begin("Peripheral Group", &open);
	if (!GameLinker::IsConnected())
	{
		ImGui::Text("%s", "Turing Complete program not found!");
		ImGui::SameLine();
		if (ImGui::Button("Refresh"))
			GameLinker::ConnectToTC();
	}
	else if (IsBound())
		ImGui::Text("Bound at: 0x%p", reinterpret_cast<void*>(boundAddress));
	else
		ImGui::Text("%s", "Unbound, press \"Bind\" button");

	s32 item = !tryBindGui;
	const char *items[] = {"auto", "manual"};
	ImGui::ListBox("Bind Type", &item, items, sizeof(items)/sizeof(items[0]));
	tryBindGui = !item;
	if (tryBindGui)
		ImGui::InputText("Pattern to search", &patternAsText);
	else
		ImGui::InputText("Address", &addressAsText, ImGuiInputTextFlags_CharsHexadecimal);

	bool recompute = false;
	if (ImGui::Button("Bind"))
	{
		if (tryBindGui)
		{
			ParsePatternText();
			SearchForPattern(targetPattern);
		}
		else
		{
			u32 address = std::atoi(addressAsText.c_str());
			BindAt(address);
			recompute = true;
		}
	}

	for (size_t i = 0; i < peripherals.size(); i++)
	{
		bool visible = true;
		recompute |= peripherals[i]->DrawGui(visible);
		if (!visible)
		{
			delete peripherals[i];
			for (size_t j = i+1; j < peripherals.size(); j++)
				peripherals[j-1] = peripherals[j];
			peripherals.pop_back();
			i--;
		}
	}

	ImGui::End();
	if (recompute)
		UpdatePeripheralsAddress();
	open |= recompute;

	return !open;
}

bool PeripheralGroup::IsBound() const
{
	return (tryAutoBind && bindAttempts >= 8) || (!tryAutoBind && boundAddress);
}

void PeripheralGroup::AddPeripheral(Peripheral *periph)
{
	peripherals.push_back(periph);
}

void PeripheralGroup::UpdatePatternText()
{
	patternAsText.clear();
	for (size_t i = 0; i < targetPattern.size()*2; i++)
	{
		u8 val = targetPattern[i / 2];
		if ((i & 0x1) == 0)
		{
			val >>= 4;
			if (!patternAsText.empty())
				patternAsText.push_back(' ');
		}
		val &= 0xf;
		if (val < 10)
			val += '0';
		else
			val += 'a' - 10;
		patternAsText.push_back(val);
	}
}

void PeripheralGroup::ParsePatternText()
{
	targetPattern.clear();
	u8 last = 0;
	bool hasHalf = false;
	for (size_t i = 0; i < patternAsText.size(); i++)
	{
		u8 val = patternAsText[i];
		u8 res;
		if (val >= '0' && val <= '9')
			res = val - '0';
		else if (val >= 'a' && val <= 'f')
			res = val - 'a' + 10;
		else if (val >= 'A' && val <= 'F')
			res = val - 'A' + 10;
		else
			continue;

		if (hasHalf)
		{
			res |= last << 4;
			targetPattern.push_back(res);
		}
		else
			last = res;
		hasHalf = !hasHalf;
	}
	if (hasHalf)
		targetPattern.push_back(last);
}

void PeripheralGroup::UpdatePeripheralsAddress()
{
	u64 start = boundAddress;
	for (size_t i = 0; i < peripherals.size(); i++)
	{
		peripherals[i]->SetAddress(start);
		start -= peripherals[i]->GetSize();
	}
}
