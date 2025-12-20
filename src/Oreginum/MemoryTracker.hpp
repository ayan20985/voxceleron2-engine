#pragma once
#include <atomic>
#include <string>
#include "LoggerMacros.hpp"

namespace Oreginum
{
	namespace MemoryTracker
	{
		// Simple memory usage tracking for debugging
		extern std::atomic<size_t> total_allocated;
		extern std::atomic<size_t> total_deallocated;
		extern std::atomic<size_t> current_usage;
		extern std::atomic<size_t> peak_usage;
		extern std::atomic<size_t> allocation_count;

		void reset();
		void log_stats();

		// Memory tracking macros
		#define TRACK_ALLOCATION(size, type) \
			do { \
				Oreginum::MemoryTracker::total_allocated += size; \
				Oreginum::MemoryTracker::current_usage += size; \
				Oreginum::MemoryTracker::allocation_count++; \
				size_t peak = Oreginum::MemoryTracker::peak_usage.load(); \
				while (!Oreginum::MemoryTracker::peak_usage.compare_exchange_weak(peak, std::max(peak, Oreginum::MemoryTracker::current_usage.load()))) {} \
				LOG_TRACE("Allocated " + std::to_string(size) + " bytes for " + std::string(type)); \
			} while(0)

		#define TRACK_DEALLOCATION(size, type) \
			do { \
				Oreginum::MemoryTracker::total_deallocated += size; \
				Oreginum::MemoryTracker::current_usage -= size; \
				LOG_TRACE("Deallocated " + std::to_string(size) + " bytes for " + std::string(type)); \
			} while(0)

		// RAII wrapper for tracking allocations
		template<typename T>
		class TrackedPointer
		{
		public:
			TrackedPointer(T* ptr = nullptr, size_t size = 0, const std::string& type = "")
				: ptr(ptr), size(size), type(type)
			{
				if (ptr && size > 0) TRACK_ALLOCATION(size, type.c_str());
			}

			~TrackedPointer()
			{
				if (ptr && size > 0) TRACK_DEALLOCATION(size, type.c_str());
			}

			T* get() const { return ptr; }
			size_t get_size() const { return size; }

		private:
			T* ptr;
			size_t size;
			std::string type;
		};
	}
}
