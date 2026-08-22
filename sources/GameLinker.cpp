#include <GameLinker.hpp>

// TODO: implement that for linux and macos
#ifdef _WIN32
#include <Windows.h>
#include <tlhelp32.h>
#endif

#include <string>

namespace GameLinker
{
#ifdef _WIN32
	HANDLE tcHandle = NULL;
#endif

	bool ConnectToTC()
	{
#ifdef _WIN32
		PROCESSENTRY32 entry;
		entry.dwSize = sizeof(PROCESSENTRY32);
		HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

		const std::wstring target = L"Turing Complete.exe";

		if (Process32FirstW(snapshot, &entry) == TRUE)
		{
			while (Process32NextW(snapshot, &entry) == TRUE)
			{
				std::wstring str = entry.szExeFile;
				
				if (target.compare(str) == 0)
				{
					tcHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, entry.th32ProcessID);
					break;
				}
			}
		}

		CloseHandle(snapshot);
		return tcHandle != NULL;
#else
		return false
#endif
	}

	bool IsConnected()
	{
#ifdef _WIN32
		return tcHandle != NULL;
#else
		return false;
#endif
	}

	u64 FindInProgram(std::vector<u8> pattern, u64 startAddress)
	{
		if (!IsConnected())
			return 0;

#ifdef _WIN32
		LPVOID address = reinterpret_cast<void*>(startAddress);

#define BUFFER_SIZE 4096

		u8 tempBuffer[BUFFER_SIZE];
		while (true)
		{
			MEMORY_BASIC_INFORMATION infos = {};
			if (VirtualQueryEx(tcHandle, address, &infos, sizeof(infos)) != sizeof(infos))
				break;
			LPVOID start = infos.BaseAddress;
			if (start < address)
				start = address;

			address = reinterpret_cast<u8*>(infos.BaseAddress) + infos.RegionSize;
			LPVOID end = address;
			if (infos.State != MEM_COMMIT || (infos.Protect & (PAGE_EXECUTE_READWRITE | PAGE_READWRITE)) == 0)
				continue;

			u64 startA = 0;
			u64 startB = pattern.size();
			while (start < end)
			{
				if (reinterpret_cast<u64>(start) + startA > 0x29789418000)
					int deez = 0;
				u64 dif = reinterpret_cast<u8*>(end) - reinterpret_cast<u8*>(start);
				if (dif > BUFFER_SIZE) dif = BUFFER_SIZE;
				u64 nbRead;
				if (!ReadProcessMemory(tcHandle, start, tempBuffer, dif, &nbRead) || nbRead != dif)
					break;
				for (u64 i = 0; i < nbRead; i++)
				{
					if (tempBuffer[i] == pattern[startA])
					{
						startA++;
						if (startA == pattern.size())
							return reinterpret_cast<u64>(start) + i + 1 - pattern.size();
					}
					else
						startA = 0;

					if (tempBuffer[i] == pattern[startB-1])
					{
						startB--;
						if (startB == 0)
							return reinterpret_cast<u64>(start) + i + 1 - pattern.size();
					}
					else
						startB = pattern.size();
				}
				start = reinterpret_cast<u8*>(start) + dif;
			}
		}
#endif

		return 0;
	}

	bool ReadMemory(u64 target, void *buffer, u64 bufferSize)
	{
		if (!IsConnected())
			return false;

#ifdef _WIN32
		u64 nbRead;
		return ReadProcessMemory(tcHandle, reinterpret_cast<void*>(target), buffer, bufferSize, &nbRead) && nbRead == bufferSize;
#else
		return false;
#endif
	}

	bool WriteMemory(u64 target, const void *buffer, u64 bufferSize)
	{
		if (!IsConnected())
			return false;

#ifdef _WIN32
		u64 nbRead;
		bool res = WriteProcessMemory(tcHandle, reinterpret_cast<void*>(target), buffer, bufferSize, &nbRead) && nbRead == bufferSize;
		FlushProcessWriteBuffers();
		return res;
#else
		return false;
#endif
	}
}