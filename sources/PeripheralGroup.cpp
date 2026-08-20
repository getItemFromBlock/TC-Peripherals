#include <PeripheralGroup.hpp>
#include <GameLinker.hpp>
#include <SDL3/SDL.h>

PeripheralGroup::~PeripheralGroup()
{
	for (size_t i = 0; i < peripherals.size(); i++)
	{
		delete peripherals[i];
	}
	peripherals.clear();
}

void PeripheralGroup::SearchForPattern(const std::vector<u8>& pattern, u64 start)
{
	tryAutoBind = true;
	bindAttempts = 0;
	startAddress = start;
	boundAddress = 0;
	targetPattern = pattern;
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
	if (tryAutoBind && bindAttempts < 4 && targetPattern.size() > 0)
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
				SDL_Log("Could not find pattern in program!");
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
			if (buf)
				delete[] buf;
			if (success)
			{
				bindAttempts++;
				if (bindAttempts == 4)
				{
					peripherals.back()->SetAddress(boundAddress);
				}
			}
			else
				bindAttempts = 0;
		}
	}
}

bool PeripheralGroup::IsBound() const
{
	return (tryAutoBind && bindAttempts >= 4) || (!tryAutoBind && boundAddress);
}

void PeripheralGroup::AddPeripheral(Peripheral *periph)
{
	peripherals.push_back(periph);
}
