#include "Logger.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <chrono>
#include <windows.h>

namespace Oreginum
{
    // Static member definitions
    std::mutex Logger::output_mutex;
    bool Logger::enabled = true;

    void Logger::log(Level level, const std::string& message)
    {
        if (!enabled) return;

        std::lock_guard<std::mutex> lock(output_mutex);
        
        std::string timestamp = get_timestamp();
        std::string level_str = level_to_string(level);
        
        // Format: [HH:MM:SS] [LEVEL] MESSAGE
        std::stringstream formatted;
        formatted << "[" << timestamp << "] [" << level_str << "] " << message;
        
        output_to_console(formatted.str());
    }

    void Logger::info(const std::string& message)
    {
        log(Level::INFO, message);
    }

    void Logger::warn(const std::string& message)
    {
        log(Level::WARN, message);
    }

    void Logger::excep(const std::string& message)
    {
        log(Level::EXCEP, message);
    }

    void Logger::set_enabled(bool enable)
    {
        std::lock_guard<std::mutex> lock(output_mutex);
        enabled = enable;
    }

    bool Logger::is_enabled()
    {
        return enabled;
    }

    std::string Logger::get_timestamp()
    {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        
        std::tm local_time;
        #ifdef _WIN32
            localtime_s(&local_time, &time_t);
        #else
            local_time = *std::localtime(&time_t);
        #endif
        
        std::stringstream ss;
        ss << std::put_time(&local_time, "%H:%M:%S");
        return ss.str();
    }

    std::string Logger::level_to_string(Level level)
    {
        switch (level)
        {
            case Level::INFO:  return "INFO ";
            case Level::WARN:  return "WARN ";
            case Level::EXCEP: return "EXCEP";
            default:           return "UNKN ";
        }
    }

    void Logger::output_to_console(const std::string& formatted_message)
    {
        // Use Windows console API for better control
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        if (hConsole != INVALID_HANDLE_VALUE)
        {
            DWORD written;
            std::string output = formatted_message + "\n";
            WriteConsoleA(hConsole, output.c_str(), static_cast<DWORD>(output.length()), &written, NULL);
        }
        else
        {
            // Fallback to standard output
            std::cout << formatted_message << std::endl;
        }
    }
}