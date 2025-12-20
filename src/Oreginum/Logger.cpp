#include "Logger.hpp"
#include <iostream>
#include <iomanip>
#include <filesystem>
#include <vulkan/vulkan.hpp>

namespace
{
	std::mutex logger_mutex;
	std::ofstream log_file;
	Oreginum::Logger::Level minimum_level{Oreginum::Logger::Level::INFO};
	bool console_output_enabled{true};
	bool initialized{false};
}

namespace Oreginum
{
	namespace Logger
	{
		void initialize(const std::string& log_file_path, Level min_level, bool console_output)
		{
			std::lock_guard<std::mutex> lock{logger_mutex};

			if (initialized)
			{
				if (log_file.is_open()) log_file.close();
			}

			minimum_level = min_level;
			console_output_enabled = console_output;

			// Generate timestamped filename
			auto now = std::chrono::system_clock::now();
			auto time_t_now = std::chrono::system_clock::to_time_t(now);
			std::tm tm_now;
			localtime_s(&tm_now, &time_t_now);

			std::filesystem::path log_path{log_file_path};
			std::filesystem::path log_dir = log_path.parent_path();
			std::filesystem::path log_filename = log_path.filename();
			std::filesystem::path log_stem = log_filename.stem();
			std::filesystem::path log_ext = log_filename.extension();

			// Create timestamp string: YYYY-MM-DD_HH-MM-SS
			std::stringstream timestamp_ss;
			timestamp_ss << std::put_time(&tm_now, "%Y-%m-%d_%H-%M-%S");
			std::string timestamp = timestamp_ss.str();

			// Create timestamped filename: stem_timestamp.ext
			std::string timestamped_filename = log_stem.string() + "_" + timestamp + log_ext.string();
			std::filesystem::path final_log_path = log_dir / timestamped_filename;

			// Create log directory if it doesn't exist
			if (!log_dir.empty() && !std::filesystem::exists(log_dir))
			{
				std::filesystem::create_directories(log_dir);
			}

			log_file.open(final_log_path, std::ios::out | std::ios::trunc);
			if (!log_file.is_open())
			{
				std::cerr << "Failed to open log file: " << final_log_path << std::endl;
				return;
			}

			initialized = true;

			// Write initialization message (reuse timestamp from above)

			log_file << "=== Voxceleron2 Engine Log Started ===" << std::endl;
			log_file << "Started at: " << std::put_time(&tm_now, "%Y-%m-%d %H:%M:%S") << std::endl;
			log_file << "Minimum log level: " << static_cast<int>(min_level) << std::endl;
			log_file << "======================================" << std::endl << std::endl;

			if (console_output)
			{
				std::cout << "Logger initialized. Log file: " << final_log_path << std::endl;
			}
		}

		void destroy()
		{
			std::lock_guard<std::mutex> lock{logger_mutex};

			if (!initialized) return;

			auto now = std::chrono::system_clock::now();
			auto time_t_now = std::chrono::system_clock::to_time_t(now);
			std::tm tm_now;
			localtime_s(&tm_now, &time_t_now);

			log_file << std::endl;
			log_file << "======================================" << std::endl;
			log_file << "Voxceleron2 Engine Log Ended" << std::endl;
			log_file << "Ended at: " << std::put_time(&tm_now, "%Y-%m-%d %H:%M:%S") << std::endl;
			log_file << "======================================" << std::endl;

			if (log_file.is_open()) log_file.close();
			initialized = false;
		}

		std::string level_to_string(Level level)
		{
			switch (level)
			{
			case Level::TRACE: return "TRACE";
			case Level::DEBUG: return "DEBUG";
			case Level::INFO: return "INFO";
			case Level::WARNING: return "WARN";
			case Level::ERRO: return "ERROR";
			case Level::FATAL: return "FATAL";
			default: return "UNKNOWN";
			}
		}

		void write_log_entry(const LogEntry& entry)
		{
			if (!initialized) return;

			auto time_t_timestamp = std::chrono::system_clock::to_time_t(entry.timestamp);
			std::tm tm_timestamp;
			localtime_s(&tm_timestamp, &time_t_timestamp);

			auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
				entry.timestamp.time_since_epoch()) % 1000;

			std::stringstream ss;
			ss << "[" << std::put_time(&tm_timestamp, "%H:%M:%S") << "." << std::setfill('0')
			   << std::setw(3) << milliseconds.count() << "] ";
			ss << "[" << level_to_string(entry.level) << "] ";
			ss << "[" << std::this_thread::get_id() << "] ";

			if (!entry.file.empty())
			{
				std::filesystem::path file_path{entry.file};
				ss << "[" << file_path.filename().string() << ":" << entry.line << "] ";
			}

			if (!entry.function.empty())
			{
				ss << "[" << entry.function << "] ";
			}

			ss << entry.message << std::endl;

			std::string log_line = ss.str();

			// Write to file
			if (log_file.is_open())
			{
				log_file << log_line;
				log_file.flush();
			}

			// Write to console if enabled
			if (console_output_enabled)
			{
				if (entry.level >= Level::WARNING)
				{
					std::cerr << log_line;
				}
				else
				{
					std::cout << log_line;
				}
			}
		}

		void log(Level level, const std::string& file, int line,
			const std::string& function, const std::string& message)
		{
			if (!initialized || level < minimum_level) return;

			LogEntry entry{
				std::chrono::system_clock::now(),
				level,
				file,
				line,
				function,
				std::this_thread::get_id(),
				message
			};

			std::lock_guard<std::mutex> lock{logger_mutex};
			write_log_entry(entry);
		}

		void trace(const std::string& message, const std::string& file,
			int line, const std::string& function)
		{
			log(Level::TRACE, file, line, function, message);
		}

		void debug(const std::string& message, const std::string& file,
			int line, const std::string& function)
		{
			log(Level::DEBUG, file, line, function, message);
		}

		void info(const std::string& message, const std::string& file,
			int line, const std::string& function)
		{
			log(Level::INFO, file, line, function, message);
		}

		void warning(const std::string& message, const std::string& file,
			int line, const std::string& function)
		{
			log(Level::WARNING, file, line, function, message);
		}

		void error(const std::string& message, const std::string& file,
			int line, const std::string& function)
		{
			log(Level::ERRO, file, line, function, message);
		}

		void fatal(const std::string& message, const std::string& file,
			int line, const std::string& function)
		{
			log(Level::FATAL, file, line, function, message);
		}

		// Timer implementation
		Timer::Timer(const std::string& name, const std::string& file,
			int line, const std::string& function)
			: name(name), file(file), line(line), function(function),
			  start_time(std::chrono::high_resolution_clock::now())
		{
			Logger::debug("Timer '" + name + "' started", file, line, function);
		}

		Timer::~Timer()
		{
			auto end_time = std::chrono::high_resolution_clock::now();
			auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
				end_time - start_time);

			std::stringstream ss;
			ss << "Timer '" << name << "' completed in " << duration.count() << " microseconds";
			Logger::debug(ss.str(), file, line, function);
		}

		// Vulkan result checking
		template<typename T>
		void check_vk_result(T result, const std::string& operation,
			const std::string& file, int line, const std::string& function)
		{
			if (result != vk::Result::eSuccess)
			{
				std::stringstream ss;
				ss << "Vulkan operation '" << operation << "' failed with result: "
				   << vk::to_string(result);
				Logger::error(ss.str(), file, line, function);
			}
		}

		// Explicit template instantiation for vk::Result
		template void check_vk_result<vk::Result>(vk::Result result,
			const std::string& operation, const std::string& file, int line, const std::string& function);
	}
}
