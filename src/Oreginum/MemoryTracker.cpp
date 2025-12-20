#include "MemoryTracker.hpp"

namespace Oreginum
{
	namespace MemoryTracker
	{
		std::atomic<size_t> total_allocated{0};
		std::atomic<size_t> total_deallocated{0};
		std::atomic<size_t> current_usage{0};
		std::atomic<size_t> peak_usage{0};
		std::atomic<size_t> allocation_count{0};

		void reset()
		{
			total_allocated = 0;
			total_deallocated = 0;
			current_usage = 0;
			peak_usage = 0;
			allocation_count = 0;
			LOG_DEBUG("Memory tracker reset");
		}

		void log_stats()
		{
			size_t allocated = total_allocated.load();
			size_t deallocated = total_deallocated.load();
			size_t current = current_usage.load();
			size_t peak = peak_usage.load();
			size_t count = allocation_count.load();

			LOG_INFO("Memory Statistics:");
			LOG_INFO("  Total allocated: " + std::to_string(allocated) + " bytes");
			LOG_INFO("  Total deallocated: " + std::to_string(deallocated) + " bytes");
			LOG_INFO("  Current usage: " + std::to_string(current) + " bytes");
			LOG_INFO("  Peak usage: " + std::to_string(peak) + " bytes");
			LOG_INFO("  Active allocations: " + std::to_string(count));

			if (allocated != deallocated)
			{
				LOG_WARNING("Memory leak detected: " + std::to_string(allocated - deallocated) + " bytes");
			}
		}
	}
}
