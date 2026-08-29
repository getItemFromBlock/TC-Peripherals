#include <peripherals/Controller.hpp>

#include <SDL3/SDL.h>
#include <imgui/imgui.h>

Controller::Controller()
{
}

Controller::~Controller()
{
	Peripheral::~Peripheral();
}

void Controller::Update()
{
}

bool Controller::DrawGui(bool &visible)
{
	const char *dataTypes[] = {"byte", "short", "word", "float"};
	bool open;
	bool result = DrawGuiBase("Speaker", open, visible);
	if (open)
	{
		ImGui::Text("Gamepad state: %s", HasGamepad() ? "CONNECTED" : "DISCONNECTED");

		if (ImGui::Button("Scan for gamepads"))
		{
			
		}

		DrawGuiEnd();
	}
	return result;
}

u64 Controller::GetSize()
{
	return 0;
}

bool Controller::HasGamepad() const
{
	return selectedGamepad != (u32)-1;
}
