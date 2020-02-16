#include "Logger.h"
#include <Engine/Memory/Allocator.h>
#include <Windows.h>
#include <filesystem>
#include <iostream>
#include <cstdarg>

namespace fs = std::filesystem;

frostwave::Logger* frostwave::Logger::m_Instance = nullptr;

void frostwave::Logger::Create()
{
	m_Instance = Allocate();
	m_Instance->InitConsole();
	m_Instance->m_Buffer = Allocate(10KB);
}

void frostwave::Logger::Destroy()
{
	Free(m_Instance->m_Buffer);
	Free(m_Instance);
	m_Instance = nullptr;
}

void frostwave::Logger::SetLevel(Level level)
{
	m_Instance->m_Level = level;
}

void frostwave::Logger::Log(Level level, const c8* file, u32 line, const c8* function, const c8* format, ...)
{
#ifdef _RETAIL
	level; file; line; function; format;
	return;
#else
	if (!m_Instance)
	{
		c8 buffer[512];

		va_list args;
		va_start(args, format);
		vsprintf_s(buffer, 512, format, args);
		va_end(args);

		printf("%s\n", buffer);

		return;
	}

	if ((c8)level < (c8)m_Instance->m_Level) return;
	if (level == Level::All || level == Level::Count) return;

	std::string filename = fs::path(file).filename().string();

	std::string func(function);
	if (func.find("lambda") != std::string::npos) func = func.substr(0, func.find("lambda") + 6) + ">";

	if (u64 pos = func.find_last_of(":"); pos != std::string::npos)
	{
		func = func.substr(pos + 1);
	}

	c8* buffer = m_Instance->m_Buffer;

	va_list args;
	va_start(args, format);
	vsprintf_s(buffer, 10KB, format, args);
	va_end(args);

	HWND hwnd = GetConsoleWindow();
	if (level >= Level::Error)
	{
		FlashWindow(hwnd, false);
	}

	HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(console, DarkTextColorIndex);
	printf("[");
	SetConsoleTextAttribute(console, (c8)level);
	printf("%s", GetLevelString(level));
	SetConsoleTextAttribute(console, DarkTextColorIndex);
	printf("] %s:%d:%s: ", filename.c_str(), line, func.c_str());
	SetConsoleTextAttribute(console, WhiteTextColorIndex);
	printf("%s\n", buffer);



	if (level == Level::Fatal) abort();
#endif
}

bool frostwave::Logger::Valid()
{
	return m_Instance && m_Instance->m_Buffer;
}

frostwave::Logger::Logger() : m_Buffer(nullptr), m_Level(Level::All)
{
}

frostwave::Logger::~Logger()
{
}

void frostwave::Logger::InitConsole()
{
#ifdef _RETAIL
	return;
#else
#pragma warning( push )
#pragma warning( disable : 4996 )
	AllocConsole();
	FILE* f = nullptr;
	f = freopen("CONIN$", "r", stdin);
	f = freopen("CONOUT$", "w", stdout);
	f = freopen("CONOUT$", "w", stderr);

	setbuf(stdin, NULL);
	setbuf(stdout, NULL);
	setbuf(stderr, NULL);
#pragma warning( pop )

	HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);

	_CONSOLE_SCREEN_BUFFER_INFOEX csbi;
	csbi.cbSize = sizeof(_CONSOLE_SCREEN_BUFFER_INFOEX);
	GetConsoleScreenBufferInfoEx(console, &csbi);

	csbi.ColorTable[0] = (25 << 0) | (25 << 8) | (30 << 16);
	csbi.ColorTable[DarkTextColorIndex] = (150 << 0) | (150 << 8) | (150 << 16);
	csbi.ColorTable[InfoColorIndex] = (0 << 0) | (150 << 8) | (75 << 16);
	csbi.ColorTable[WarningColorIndex] = (200 << 0) | (200 << 8) | (50 << 16);
	csbi.ColorTable[ErrorColorIndex] = (255 << 0) | (50 << 8) | (50 << 16);
	csbi.ColorTable[WhiteTextColorIndex] = (219 << 0) | (221 << 8) | (231 << 16);
	csbi.cbSize = sizeof(csbi);
	SetConsoleScreenBufferInfoEx(console, &csbi);

	HWND hwnd = GetConsoleWindow();

	SetConsoleTitle(L"Frostwave Console");
	SetWindowLong(hwnd, GWL_EXSTYLE, GetWindowLong(hwnd, GWL_EXSTYLE) | WS_EX_LAYERED);
	SetLayeredWindowAttributes(hwnd, 0, ConsoleAlpha, LWA_ALPHA);

	MoveWindow(hwnd, 0, 0, 1250, 600, true);
#endif
}

const char* frostwave::Logger::GetLevelString(Level level)
{
	switch (level)
	{
	case frostwave::Logger::Level::Verbose:
		return "VERBOSE";
		break;
	case frostwave::Logger::Level::Info:
		return "INFO";
		break;
	case frostwave::Logger::Level::Warning:
		return "WARNING";
		break;
	case frostwave::Logger::Level::Error:
		return "ERROR";
		break;
	case frostwave::Logger::Level::Fatal:
		return "FATAL";
		break;
	default:
		return "???";
		break;
	}
}