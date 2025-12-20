#pragma once
#include "Logger.hpp"

// Logging macros that automatically capture file, line, and function information
#define LOG_TRACE(message) Oreginum::Logger::trace(message, __FILE__, __LINE__, __FUNCTION__)
#define LOG_DEBUG(message) Oreginum::Logger::debug(message, __FILE__, __LINE__, __FUNCTION__)
#define LOG_INFO(message) Oreginum::Logger::info(message, __FILE__, __LINE__, __FUNCTION__)
#define LOG_WARNING(message) Oreginum::Logger::warning(message, __FILE__, __LINE__, __FUNCTION__)
#define LOG_ERROR(message) Oreginum::Logger::error(message, __FILE__, __LINE__, __FUNCTION__)
#define LOG_FATAL(message) Oreginum::Logger::fatal(message, __FILE__, __LINE__, __FUNCTION__)

// Performance timing macro
#define LOG_TIMER(name) Oreginum::Logger::Timer timer##__LINE__(name, __FILE__, __LINE__, __FUNCTION__)

// Vulkan result checking macro
#define VK_CHECK(result, operation) Oreginum::Logger::check_vk_result(result, operation, __FILE__, __LINE__, __FUNCTION__)

// Assert macro that logs before asserting
#define LOG_ASSERT(condition, message) \
	do { \
		if (!(condition)) { \
			LOG_FATAL("Assertion failed: " + std::string(message) + " - Condition: " #condition); \
			assert(condition); \
		} \
	} while (0)
