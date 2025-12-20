#include <string>
#include <thread>
#include "../Oreginum/Window.hpp"
#include "../Oreginum/Core.hpp"
#include "../Oreginum/Camera.hpp"
#include "../Oreginum/Mouse.hpp"
#include "../Oreginum/Keyboard.hpp"
#include "../Oreginum/LoggerMacros.hpp"
#include "World.hpp"

int WinMain(HINSTANCE current, HINSTANCE previous, LPSTR arguments, int show)
{
	try
	{
		//Set a high thread priority
		SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
		SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);

		LOG_INFO("Setting process priority to HIGH_PRIORITY_CLASS");
		LOG_INFO("Setting main thread priority to THREAD_PRIORITY_HIGHEST");

		//Initialize
		Oreginum::Core::initialize("Voxceleron2", {1920, 1080}, false);
		Tetra::World world{};

		LOG_INFO("Entering main game loop");

		//Main loop
		while(Oreginum::Core::update())
		{
			//Mouse lock toggle
			if(Oreginum::Keyboard::was_pressed(Oreginum::Key::L))
			{
				bool new_locked = !Oreginum::Mouse::is_locked();
				Oreginum::Mouse::set_locked(new_locked);
				LOG_DEBUG("Mouse lock toggled: " + std::string(new_locked ? "locked" : "unlocked"));
			}

			world.update();
		}

		LOG_INFO("Main game loop exited");
		Oreginum::Core::destroy();
	}
	catch (const std::exception& e)
	{
		LOG_FATAL("Unhandled exception in main: " + std::string(e.what()));
		Oreginum::Core::fatal_error("Unhandled exception: " + std::string(e.what()));
	}
	catch (...)
	{
		LOG_FATAL("Unhandled unknown exception in main");
		Oreginum::Core::fatal_error("Unhandled unknown exception");
	}

	return 0;
}