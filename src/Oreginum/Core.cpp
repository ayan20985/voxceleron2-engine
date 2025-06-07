#include <chrono>
#define NOMINMAX
#include <windows.h>
#include "Window.hpp"
#include "Keyboard.hpp"
#include "Renderer Core.hpp"
#include "Mouse.hpp"
#include "Camera.hpp"
#include "Core.hpp"
#include "Main Renderer.hpp"
#include "Logger.hpp"

namespace
{
	glm::ivec2 screen_resolution;
	uint32_t refresh_rate;
	float previous_time, delta;
	float minimum_delta;
	static double initial_time;
	bool vsync, debug;
	bool console_allocated = false;
	Oreginum::Logger::Verbosity log_verbosity = Oreginum::Logger::Verbosity::NORMAL;

	double time_since_epoch(){ return std::chrono::duration_cast<std::chrono::microseconds>
		(std::chrono::high_resolution_clock::now().time_since_epoch()).count()/1000000.; }
};

void Oreginum::Core::initialize(const std::string& title,
	const glm::ivec2& resolution, bool vsync, bool terminal, bool debug, 
	Logger::Verbosity log_verbosity)
{
	::vsync = vsync;
	::debug = debug;
	::log_verbosity = log_verbosity;
	
	// Allocate console if requested
	if (terminal) {
		AllocConsole();
		console_allocated = true;
		FILE* pCout;
		freopen_s(&pCout, "CONOUT$", "w", stdout);
		FILE* pCin;
		freopen_s(&pCin, "CONIN$", "r", stdin);
		FILE* pCerr;
		freopen_s(&pCerr, "CONOUT$", "w", stderr);
		std::ios::sync_with_stdio(true);
		std::wcout.clear();
		std::cout.clear();
		std::wcerr.clear();
		std::cerr.clear();
		std::wcin.clear();
		std::cin.clear();
	}
	
	// Initialize logger
	Logger::set_enabled(true);
	Logger::set_verbosity(log_verbosity);
	Logger::info("Initializing Voxceleron2 Engine...", true);
	
	//srand(static_cast<unsigned>(time(NULL)));
	screen_resolution = {GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN)};
	DEVMODE devmode;
	EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &devmode);
	refresh_rate = devmode.dmDisplayFrequency;
	minimum_delta = 1.f/get_refresh_rate();
	initial_time = time_since_epoch();

	Logger::info("Screen resolution: " + std::to_string(screen_resolution.x) + "x" + std::to_string(screen_resolution.y), true);
	Logger::info("Refresh rate: " + std::to_string(refresh_rate) + "Hz", true);

	Window::initialize(title, resolution, terminal);
	Logger::info("Window initialized", true);
	
	Mouse::initialize();
	Logger::info("Mouse system initialized", true);
	
	Renderer_Core::initialize();
	Logger::info("Renderer core initialized", true);
	
	Logger::info("Engine initialization complete", true);
}

void Oreginum::Core::destroy()
{
	Logger::info("Shutting down engine...", true);
	Mouse::destroy();
	Window::destroy();
	Renderer_Core::get_device()->get().waitIdle();
	
	// Clean up console if we allocated it
	if (console_allocated) {
		Logger::info("Engine shutdown complete. Press any key to close console...", true);
		std::cin.get();
		FreeConsole();
	}
}

void Oreginum::Core::error(const std::string& error)
{
	Logger::excep("FATAL ERROR: " + error);
	destroy();
	MessageBox(NULL, error.c_str(), "Oreginum Engine Error", MB_ICONERROR);
	std::exit(EXIT_FAILURE);
}

bool Oreginum::Core::update()
{
	delta = get_time()-previous_time;
	if(vsync)
	{
		timeBeginPeriod(1);
		while((delta = get_time()-previous_time) < minimum_delta)
			if(minimum_delta-delta < .003f) Sleep(0); else Sleep(1);
		timeEndPeriod(1);
	}
	previous_time = get_time();

	Mouse::update();
	Keyboard::update();
	Window::update();
    Camera::update();

	std::lock_guard<std::mutex> guard{*Oreginum::Renderer_Core::get_render_mutex()};
	Oreginum::Renderer_Core::update();
	Oreginum::Main_Renderer::render();

	return !Window::was_closed();
}

uint32_t Oreginum::Core::get_refresh_rate(){ return refresh_rate; }

const glm::ivec2& Oreginum::Core::get_screen_resolution(){ return screen_resolution; }

float Oreginum::Core::get_time(){ return static_cast<float>(time_since_epoch()-initial_time); }

float Oreginum::Core::get_delta(){ return delta; }

bool Oreginum::Core::get_debug(){ return debug; }

void Oreginum::Core::set_log_verbosity(Logger::Verbosity level)
{
	log_verbosity = level;
	Logger::set_verbosity(level);
}

Oreginum::Logger::Verbosity Oreginum::Core::get_log_verbosity()
{
	return log_verbosity;
}