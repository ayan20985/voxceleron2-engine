#pragma once
#include <string>
#include <mutex>
#include <fstream>
#include <chrono>
#include <thread>
#include <sstream>

namespace Oreginum
{
	namespace Logger
	{
	enum class Level
	{
		TRACE = 0,
		DEBUG = 1,
		INFO = 2,
		WARNING = 3,
		ERRO = 4,
		FATAL = 5
	};

		struct LogEntry
		{
			std::chrono::system_clock::time_point timestamp;
			Level level;
			std::string file;
			int line;
			std::string function;
			std::thread::id thread_id;
			std::string message;
		};

		void initialize(const std::string& log_file_path = "voxceleron2.log",
			Level min_level = Level::INFO, bool console_output = true);
		void destroy();

		void log(Level level, const std::string& file, int line,
			const std::string& function, const std::string& message);

		// Convenience functions for different levels
		void trace(const std::string& message, const std::string& file = "",
			int line = 0, const std::string& function = "");
		void debug(const std::string& message, const std::string& file = "",
			int line = 0, const std::string& function = "");
		void info(const std::string& message, const std::string& file = "",
			int line = 0, const std::string& function = "");
		void warning(const std::string& message, const std::string& file = "",
			int line = 0, const std::string& function = "");
		void error(const std::string& message, const std::string& file = "",
			int line = 0, const std::string& function = "");
		void fatal(const std::string& message, const std::string& file = "",
			int line = 0, const std::string& function = "");

		// Performance timing utilities
		class Timer
		{
		public:
			Timer(const std::string& name, const std::string& file = "",
				int line = 0, const std::string& function = "");
			~Timer();

		private:
			std::string name;
			std::string file;
			int line;
			std::string function;
			std::chrono::high_resolution_clock::time_point start_time;
		};

		// Vulkan result checking utility
		template<typename T>
		void check_vk_result(T result, const std::string& operation,
			const std::string& file = "", int line = 0, const std::string& function = "");
	}
}
