#define NOMINMAX
#include <windows.h>
#include "Core.hpp"
#include "Window.hpp"
#include "Keyboard.hpp"
#include "Mouse.hpp"
#include "Logger.hpp"

namespace
{
	glm::ivec2 position;
	glm::ivec2 delta;
	bool locked;
	int32_t scroll_delta{};
    bool pressed[3], first_update{true};

	glm::ivec2 get_window_center()
	{ return Oreginum::Window::get_position()+Oreginum::Window::get_resolution()/2U; }

	void center(){ SetCursorPos(get_window_center().x, get_window_center().y); }
	void lock()
	{
		ShowCursor(false);
		center();
		locked = true;
		Oreginum::Logger::info("Mouse locked and cursor hidden");
	}
	void free()
	{
		ShowCursor(true);
		locked = false;
		Oreginum::Logger::info("Mouse unlocked and cursor visible");
	}
}

void Oreginum::Mouse::set_locked(bool locked)
{
	Logger::info("Setting mouse lock state: " + std::string(locked ? "locked" : "unlocked"));
	if(locked) lock(); else free();
}

void Oreginum::Mouse::initialize()
{
	Logger::info("Initializing mouse system");
	lock();
}

void Oreginum::Mouse::destroy()
{
	Logger::info("Destroying mouse system");
	free();
}

void Oreginum::Mouse::update()
{
	scroll_delta = 0;

	for(bool& b : pressed) b = false;

	if(Keyboard::was_pressed(Key::ESC))
	{
		if(locked)
		{
			Logger::info("ESC pressed - unlocking mouse");
			free();
		}
		else
		{
			Logger::info("ESC pressed - locking mouse");
			lock();
		}
	}

	//Get the new position
	static POINT point{};
	GetCursorPos(&point);
	glm::ivec2 previous_position{position};
	position = {point.x, point.y};

	//Get the delta
	if(locked)
	{
		delta = position-get_window_center();
		center();
	} else delta = position-previous_position; 

    if(first_update) delta = {}, first_update = false;
}

void Oreginum::Mouse::set_pressed(Button button, bool pressed)
{
	::pressed[button] = pressed;
	if(pressed)
	{
		std::string button_name = (button == Button::LEFT_MOUSE) ? "Left" :
								  (button == Button::RIGHT_MOUSE) ? "Right" : "Middle";
		Logger::info("Mouse button pressed: " + button_name);
	}
}

void Oreginum::Mouse::add_scroll_delta(int32_t scroll_delta)
{
	::scroll_delta += scroll_delta;
	if(scroll_delta != 0)
	{
		Logger::info("Mouse scroll: " + std::to_string(scroll_delta) + " (total: " + std::to_string(::scroll_delta) + ")");
	}
}

glm::ivec2 Oreginum::Mouse::get_position()
{
	POINT position{};
	GetCursorPos(&position);
	return glm::uvec2{position.x, position.y}-Window::get_position();
}

const glm::ivec2& Oreginum::Mouse::get_delta(){ return delta; }

int32_t Oreginum::Mouse::get_scroll_delta(){ return scroll_delta; }

bool Oreginum::Mouse::is_locked(){ return locked; }

bool Oreginum::Mouse::was_pressed(Button button){ return pressed[button]; }

bool Oreginum::Mouse::is_held(Button button)
{ return (GetAsyncKeyState(button) != 0) && Window::has_focus(); }