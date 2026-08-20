#pragma once

#include <vector>
#include <Types.hpp>

namespace GameLinker
{
	bool ConnectToTC();
	bool IsConnected();
	u64 FindInProgram(std::vector<u8> pattern, u64 startAddress);
	bool ReadMemory(u64 target, void *buffer, u64 bufferSize);
	bool WriteMemory(u64 target, const void *buffer, u64 bufferSize);
}
