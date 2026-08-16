#include <iostream>

#include <Windows.h>
#include <tlhelp32.h>
#include <thread>
#include <atomic>
#include <list>
#include <corecrt_math_defines.h>

#define MA_NO_ENGINE
#include "miniaudio.h"

#define SOUND_BUFFER_SIZE 16384


const size_t offset = 0x213404B07F0;
char* offsetPtr = reinterpret_cast<char*>(offset);


WCHAR szClassName[] = L"MainClass";
WCHAR szTitle[] = L"TC Link";

struct DataHolder
{
	//char buffer[256];
	int16_t *soundBuffer;
	std::atomic_int soundPosA;
	std::atomic_int soundPosB;
	HWND hWnd;
	HANDLE handle;
	//std::atomic_int result;
	std::atomic_bool shouldExit;
} data = {};

std::thread updateThread;


LRESULT CALLBACK WndProc(_In_ HWND hWnd, _In_ UINT message, _In_ WPARAM wParam, _In_ LPARAM lParam);
void ThreadFunc(DataHolder *data);
HANDLE FindAndOpenTC();

void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
	DataHolder *data = reinterpret_cast<DataHolder*>(pDevice->pUserData);
	int16_t *buffer = reinterpret_cast<int16_t*>(pOutput);

	for (uint32_t i = 0; i < frameCount; i++)
	{
		if (data->soundPosA == data->soundPosB)
			*buffer = 0;
		else
		{
			*buffer = data->soundBuffer[data->soundPosB++]/4;
			if (data->soundPosB >= SOUND_BUFFER_SIZE)
				data->soundPosB = 0;
		}
		buffer++;
	}
}

int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR pCmdLine, _In_ int nCmdShow)
{
#ifdef _DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	//_CrtSetBreakAlloc(353);
#endif
	{
		WNDCLASSEX wcex = {};
		wcex.cbSize = sizeof(WNDCLASSEX);
		wcex.style = CS_HREDRAW | CS_VREDRAW;
		wcex.lpfnWndProc = WndProc;
		wcex.cbClsExtra = 0;
		wcex.cbWndExtra = 0;
		wcex.hInstance = hInstance;
		wcex.hIcon = LoadIcon(wcex.hInstance, IDI_APPLICATION);
		wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
		wcex.hbrBackground = NULL;
		wcex.lpszMenuName = NULL;
		wcex.lpszClassName = szClassName;
		wcex.hIconSm = LoadIcon(wcex.hInstance, IDI_APPLICATION);

		if (!RegisterClassEx(&wcex))
		{
			MessageBox(NULL, L"Call to RegisterClassExW failed!", szTitle, NULL);
			return 1;
		}

		if (!SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_SYSTEM_AWARE))
		{
			MessageBox(NULL, L"Could not set window dpi awareness !", szTitle, NULL);
		}
		RECT r;
		r.left = 0;
		r.right = 350;
		r.top = 0;
		r.bottom = 100;
		AdjustWindowRect(&r, WS_EX_OVERLAPPEDWINDOW, true);
		HWND hWnd = CreateWindowEx(WS_EX_OVERLAPPEDWINDOW, szClassName, szTitle, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, r.right - r.left, r.bottom - r.top, NULL, NULL, hInstance, NULL);
		data.hWnd = hWnd;

		ShowWindow(hWnd, nCmdShow);
		UpdateWindow(hWnd);

		LONG_PTR lExStyle = GetWindowLongPtr(hWnd, GWL_EXSTYLE);
		lExStyle &= ~(WS_EX_DLGMODALFRAME | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE);
		SetWindowLongPtr(hWnd, GWL_EXSTYLE, lExStyle);
		SetWindowPos(hWnd, NULL, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER);
		if (!hWnd)
		{
			MessageBox(NULL, L"Call to CreateWindow failed!", szTitle, NULL);
			return 1;
		}

		data.handle = FindAndOpenTC();
		if (data.handle == NULL)
		{
			MessageBox(NULL, L"Could not find Turing Complete.exe!", szTitle, NULL);
			return 1;
		}

		data.soundBuffer = new int16_t[SOUND_BUFFER_SIZE];
		ma_device_config config = ma_device_config_init(ma_device_type_playback);
        config.playback.format   = ma_format_s16;
        config.playback.channels = 1;
        config.sampleRate        = 48000;
        config.dataCallback      = data_callback;
        config.pUserData         = &data;

        ma_device device;
        if (ma_device_init(NULL, &config, &device) != MA_SUCCESS)
		{
			MessageBox(NULL, L"Could not initialise sound output!", szTitle, NULL);
			delete[] data.soundBuffer;
            return 1;
        }
        ma_device_start(&device);

		updateThread = std::thread(&ThreadFunc, &data);

		// Main message loop:
		MSG msg;
		while (GetMessageW(&msg, NULL, 0, 0))
		{
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
		}
		data.shouldExit = true;
		ma_device_uninit(&device);
		if (updateThread.joinable())
			updateThread.join();
		CloseHandle(data.handle);
		delete[] data.soundBuffer;
		return (int)msg.wParam;
	}
}

HANDLE FindAndOpenTC()
{
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
				HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, entry.th32ProcessID);
				CloseHandle(snapshot);
				return hProcess;
			}
		}
	}

	CloseHandle(snapshot);
	return NULL;
}

LRESULT CALLBACK WndProc(_In_ HWND hWnd, _In_ UINT message, _In_ WPARAM wParam, _In_ LPARAM lParam)
{
	switch (message)
	{
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd, &ps);
        HDC memDC = CreateCompatibleDC(hdc);

        RECT rcClientRect;
        GetClientRect( hWnd, &rcClientRect);

        HBITMAP bmp = CreateCompatibleBitmap(hdc, rcClientRect.right - rcClientRect.left, rcClientRect.bottom - rcClientRect.top );
        HBITMAP oldBmp = (HBITMAP)SelectObject(memDC, bmp);

		FillRect(memDC, &rcClientRect, (HBRUSH) (COLOR_WINDOW+1));
		RECT r;
		r.left = 4;
		r.right = 100;
		r.top = 4;
		r.bottom = 50;
		DrawTextA(memDC, "TC Link Sound Peripheral", 25, &r, DT_LEFT | DT_TOP | DT_NOCLIP | DT_NOPREFIX);
		
		/*
		char buffer[256];
		int len = sprintf_s(buffer, 256, "Text: %s", data.buffer);
		DrawTextA(memDC, buffer, len, &r, DT_LEFT | DT_TOP | DT_NOCLIP | DT_NOPREFIX);

		len = sprintf_s(buffer, 256, "Result: %d", data.result.load());
		r.top = 20;
		r.bottom = 70;
		DrawTextA(memDC, buffer, len, &r, DT_LEFT | DT_TOP | DT_NOCLIP | DT_NOPREFIX);
		*/
		BitBlt( hdc, 0, 0, rcClientRect.right - rcClientRect.left, rcClientRect.bottom - rcClientRect.top, memDC, 0, 0, SRCCOPY );

        SelectObject(memDC, oldBmp);
        DeleteObject(bmp);
        DeleteDC(memDC);

        EndPaint(hWnd, &ps);
	}
		break;
	case WM_CLEAR:
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_GETMINMAXINFO:
	{
		LPMINMAXINFO lpMMI = (LPMINMAXINFO)lParam;
		lpMMI->ptMinTrackSize.x = 64;
		lpMMI->ptMinTrackSize.y = 64;
		break;
	}
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

#define BUFFER_CHUNK 512
#define BUFFER_OFFSET 16
void ThreadFunc(DataHolder *dataIn)
{
	while (!dataIn->shouldExit.load())
	{
		InvalidateRect(dataIn->hWnd,NULL,true);

		int dif = dataIn->soundPosA - dataIn->soundPosB;
		if (dif < 0)
			dif += SOUND_BUFFER_SIZE;
		if (dif > BUFFER_CHUNK * 4)
			continue;

		uint16_t flags;
		if (!ReadProcessMemory(dataIn->handle, offsetPtr - sizeof(flags), &flags, sizeof(flags), NULL))
			continue;

		if (flags == 1)
			continue;
		if (flags == 2)
		{
			int16_t tmpBuffer[BUFFER_CHUNK];
			if (!ReadProcessMemory(dataIn->handle, offsetPtr - sizeof(tmpBuffer) - BUFFER_OFFSET, &tmpBuffer, sizeof(tmpBuffer), NULL))
				continue;

			int pos = data.soundPosA;
			for (int i = 0; i < BUFFER_CHUNK; i++)
				data.soundBuffer[(pos + i) % SOUND_BUFFER_SIZE] = tmpBuffer[BUFFER_CHUNK - i - 1];

			int tmp = data.soundPosA + BUFFER_CHUNK;
			if (tmp >= SOUND_BUFFER_SIZE)
				tmp -= SOUND_BUFFER_SIZE;
			data.soundPosA = tmp;
		}

		flags = 1;
		WriteProcessMemory(dataIn->handle, offsetPtr - sizeof(flags), &flags, sizeof(flags), NULL);
	}
}

#if 0
void ThreadFunc(DataHolder *dataIn)
{
	while (!dataIn->shouldExit.load())
	{
		char buffer[256];
		if (!ReadProcessMemory(dataIn->handle, offsetPtr - sizeof(buffer), buffer, sizeof(buffer), NULL))
		{
			continue;
		}

		struct Token
		{
			int numberValue;
			int operatorValue;
		};
		std::list<Token> tokens;
		int num = 0;
		int op = 0;
		bool wasSpace = false;
		bool wasNum = false;
		bool wasOp = false;
		for (int i = sizeof(buffer)-1; i >= 0; i--)
		{
			char a = buffer[i];
			dataIn->buffer[255 - i] = a;
			bool isEnd = false;
			bool isSpace = false;
			bool isNum = false;
			bool isOp = false;
			if (a >= '0' && a <= '9')
				isNum = true;
			else if (a == ' ')
				isSpace = true;
			else if (a == '+' || a == '-' || a == '*' || a == '/' || a == '%')
				isOp = true;
			else
				isEnd = true;

			if (!wasSpace && !wasNum && !wasOp && !isEnd)
			{
				if (isSpace)
					continue;
				if (isNum)
					num = a - '0';
				else if (isOp)
					op = a;
				wasNum = isNum;
				wasOp = isOp;
			}
			else if ((isEnd && wasNum) || (isNum && wasSpace && wasNum) || (isOp && wasNum))
			{
				Token t = {};
				t.numberValue = num;
				tokens.push_back(t);

				num = 0;
				op = 0;
				if (isOp)
					op = a;
				else if (isNum)
					num = a - '0';
				
				wasSpace = false;
				wasOp = isOp;
				wasNum = isNum;
			}
			else if (isNum && wasNum)
			{
				num = num * 10 + a - '0';
			}
			else if ((isNum || isOp) && wasOp)
			{
				Token t = {};
				t.operatorValue = op;
				tokens.push_back(t);

				num = 0;
				op = 0;
				if (isOp)
					op = a;
				else if (isNum)
					num = a - '0';

				wasSpace = false;
				wasOp = isOp;
				wasNum = isNum;
			}
			else if (isSpace)
				wasSpace = true;

			if (isEnd)
				break;
		}

		int count = 0;
		int lastcount = 0;
		do
		{
			lastcount = count;
			count = 0;
			for (auto it = tokens.begin(); it != tokens.end(); it++)
			{
				count++;
				if (it->operatorValue)
				{
					auto next = std::next(it);
					if (it == tokens.begin() || next == tokens.end())
					{
						count = 0;
						break;
					}
					auto prev = std::prev(it);
					if (it->operatorValue && prev->operatorValue && next->numberValue)
					{
						if (it->operatorValue == '+')
						{
							auto tmp = prev;
							tokens.erase(it);
							it = tmp;
							count--;
							continue;
						}
						else if (it->operatorValue == '-')
						{
							next->numberValue = -next->numberValue;
							auto tmp = prev;
							tokens.erase(it);
							it = tmp;
							count--;
							continue;
						}
						else
						{
							count = 0;
							break;
						}
					}
					else if (it->operatorValue && next->numberValue && prev->numberValue)
					{
						int a = prev->numberValue;
						int b = next->numberValue;
						bool invalid = false;
						switch (it->operatorValue)
						{
						case '+':
							a = a + b;
							break;
						case '-':
							a = a - b;
							break;
						case '*':
							a = a * b;
							break;
						case '/':
							if (b == 0)
								invalid = true;
							else
								a = a / b;
							break;
						case '%':
							if (b == 0)
								invalid = true;
							else
								a = a % b;
							break;
						default:
							invalid = true;
							break;
						}

						if (invalid)
						{
							count = 0;
							break;
						}
						else
						{
							prev->numberValue = a;
							auto tmp = prev;
							tokens.erase(next);
							tokens.erase(it);
							it = tmp;
							count--;
							continue;
						}
					}
				}
			}
		} while (count > 1 && lastcount != count);

		int8_t countFinal = 0;
		if (count == 1 && !tokens.empty())
			countFinal = tokens.front().numberValue;
		else
			countFinal = 0;
		dataIn->result = countFinal;

		WriteProcessMemory(dataIn->handle, offsetPtr - sizeof(dataIn->buffer), &countFinal, 1, NULL);

		InvalidateRect(dataIn->hWnd,NULL,true);
		//RedrawWindow(dataIn->hWnd, NULL, NULL, RDW_INTERNALPAINT | RDW_INVALIDATE | RDW_ERASE);
	}
}
#endif
