#pragma once
#include "Common.hpp"
#include "Chunk.hpp"
#include <unordered_map>
#include <unordered_set>
#include "../Oreginum/Camera.hpp"
#include "../Oreginum/Renderer Core.hpp"

namespace Tetra
{
	// Hash function for glm::ivec3
	struct ivec3_hash {
		std::size_t operator()(const glm::ivec3& v) const {
			return std::hash<int>()(v.x) ^ (std::hash<int>()(v.y) << 1) ^ (std::hash<int>()(v.z) << 2);
		}
	};

	class World
	{
	public:
		World();
		~World();
		void update();

		// Infinite world specific methods
		void update_chunks_around_player();
		glm::ivec3 world_pos_to_chunk_pos(const glm::fvec3& world_pos);
		bool is_chunk_loaded(const glm::ivec3& chunk_pos);
		void load_chunk(const glm::ivec3& chunk_pos);
		void unload_chunk(const glm::ivec3& chunk_pos);

	private:
		// Infinite world data structure
		std::unordered_map<glm::ivec3, Chunk*, ivec3_hash> loaded_chunks;
		std::unordered_set<glm::ivec3, ivec3_hash> chunks_to_load;
		std::unordered_set<glm::ivec3, ivec3_hash> chunks_to_unload;
		
		// Player tracking
		glm::ivec3 current_player_chunk;
		glm::ivec3 last_player_chunk;
		
		// Render and load distances
		static constexpr int RENDER_DISTANCE = 1; // 3x3 area (1 chunk radius)
		static constexpr int LOAD_DISTANCE = 8;   // 16x16 area (8 chunk radius)
		
		// Threading
		std::vector<std::pair<Chunk *, uint8_t>> deletion_queue;
		std::vector<Chunk *> add_queue;
		std::thread threads[THREADS];
		bool is_thread_busy[THREADS];
		bool was_thread_launched[THREADS];
		bool populated, meshed;
		std::mutex add_queue_mutex, deletion_queue_mutex, chunks_mutex;

		void population_pass_1(Chunk *chunk, uint8_t thread_index);
		void population_pass_2(Chunk *chunk, uint8_t thread_index);
		void mesh_chunk(Chunk *chunk, uint8_t thread_index);
		Chunk *get_population_pass_1_chunk();
		Chunk *get_population_pass_2_chunk();
		Chunk *get_unmeshed_chunk();
		int8_t get_thread();
		float *simplex(const glm::ivec3& offset, const glm::ivec3& size,
			float frequency, uint32_t octaves, uint32_t seed);
		bool float_equals(float a, float b, float range){ return abs(a-b) < range; }
		void populate_chunk_pass_1(Chunk *chunk);
		bool axis_bounds_check(glm::u8vec3 values, glm::u8vec3 minimum, glm::u8vec3 maximum);
		void inter_chunk_set(Chunk *chunk, glm::i16vec3 voxel_index, uint8_t material);
		void create_tree(Chunk *chunk, glm::i16vec3 base_voxel_index);
		void populate_chunk_pass_2(Chunk *chunk);
		void transparent_neighbor_cull(Chunk *chunk, const glm::u8vec3& voxel_position);
		void cull_chunk(Chunk *chunk);

		// Helper functions for infinite world
		Chunk* get_chunk_at(const glm::ivec3& chunk_pos);
		bool is_chunk_in_render_distance(const glm::ivec3& chunk_pos, const glm::ivec3& player_chunk);
		bool is_chunk_in_load_distance(const glm::ivec3& chunk_pos, const glm::ivec3& player_chunk);
	};
}