#pragma once
#include <string>
#include <mutex>

namespace Oreginum
{
    class Logger
    {
    public:
        enum class Level
        {
            INFO,
            WARN,
            EXCEP
        };
        
        enum class Verbosity
        {
            MINIMAL,    // Only critical errors and warnings
            NORMAL,     // Normal operation info + errors/warnings 
            VERBOSE     // All messages including detailed debug info
        };

        // Core logging functions
        static void log(Level level, const std::string& message, bool is_critical = false);
        static void info(const std::string& message, bool is_critical = false);
        static void warn(const std::string& message);
        static void excep(const std::string& message);

        // Control functions
        static void set_enabled(bool enabled);
        static bool is_enabled();
        static void set_verbosity(Verbosity level);
        static Verbosity get_verbosity();

    private:
        static std::mutex output_mutex;
        static bool enabled;
        static Verbosity verbosity_level;

        // Internal formatting functions
        static std::string get_timestamp();
        static std::string level_to_string(Level level);
        static void output_to_console(const std::string& formatted_message);
    };
}