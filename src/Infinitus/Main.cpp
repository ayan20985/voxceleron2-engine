#include <string>
#include <thread>
#include "../Oreginum/Window.hpp"
#include "../Oreginum/Core.hpp"
#include "../Oreginum/Camera.hpp"
#include "../Oreginum/Mouse.hpp"
#include "../Oreginum/Keyboard.hpp"
#include "World.hpp"

int WinMain(HINSTANCE current, HINSTANCE previous, LPSTR arguments, int show)
{
	//Set a high thread priority
	SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);

	//Initialize
	Oreginum::Core::initialize("Voxceleron2", {1280, 720}, false);
	
	// Set initial camera position above ground level
	Oreginum::Camera::set_position(glm::fvec3(0.0f, 100.0f, 0.0f));
	
	Tetra::World world{};

	//Main loop
	while(Oreginum::Core::update())
	{
		//Mouse lock toggle
		if(Oreginum::Keyboard::was_pressed(Oreginum::Key::L))
			Oreginum::Mouse::set_locked(!Oreginum::Mouse::is_locked());

		world.update();
	}

	Oreginum::Core::destroy();
}