#pragma once
#include <array>
#include <vector>
#include <ctime>
#include <random>
#include <glm/glm.hpp>

namespace Tetra
{
	enum Materials{STONE = 1, DIRT, GRASS, SAND, WOOD, LEAVES, WATER};
	enum Axis{X, Y, Z};
	enum Faces{RIGHT, LEFT, TOP, BOTTOM, FRONT, BACK};

	// Generate a random seed each time the program starts
	inline uint32_t generateRandomSeed() {
		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_int_distribution<uint32_t> dist(1, UINT32_MAX);
		return dist(gen);
	}
	
	static uint32_t SEED = generateRandomSeed();
	const glm::uvec3 WORLD_SIZE{8, 2, 8};
	constexpr uint8_t CHUNK_SIZE{128}, CUBE_FACES{6}, THREADS{4}, CHUNKS_ADDED_PER_FRAME{1};
	constexpr uint32_t CHUNK_SIZE_CUBED{CHUNK_SIZE*CHUNK_SIZE*CHUNK_SIZE};
	static constexpr float VOXEL_SIZE{1.f};

	// LOD (Level of Detail) constants
	constexpr uint8_t LOD_LEVELS{5};
	constexpr float LOD_DISTANCE_THRESHOLDS[LOD_LEVELS]{0.f, 2.f, 4.f, 8.f, 16.f}; // Distance in chunks
	constexpr uint8_t LOD_SAMPLE_RATES[LOD_LEVELS]{1, 2, 4, 8, 16}; // Voxel sampling rates
	constexpr uint8_t MERGE_FACTOR{4}; // For LOD 4, merge 4x4x4 chunks

	struct Voxel{ uint8_t cull_mask, material; };

	static const glm::u8vec3 TREE_SIZE{5, 7, 5};
	static constexpr uint8_t TREE[7][5][5]
	{
		{{0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0},
		{0, 0, 5, 0, 0},
		{0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0}},

		{{0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0},
		{0, 0, 5, 0, 0},
		{0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0}},

		{{0, 6, 6, 6, 0},
		{6, 6, 6, 6, 6},
		{6, 6, 5, 6, 6},
		{6, 6, 6, 6, 6},
		{0, 6, 6, 6, 0}},

		{{0, 0, 6, 0, 0},
		{0, 6, 6, 6, 0},
		{6, 6, 5, 6, 6},
		{0, 6, 6, 6, 0},
		{0, 0, 6, 0, 0}},

		{{0, 0, 0, 0, 0},
		{0, 6, 6, 6, 0},
		{0, 6, 5, 6, 0},
		{0, 6, 6, 6, 0},
		{0, 0, 0, 0, 0}},

		{{0, 0, 0, 0, 0},
		{0, 0, 6, 0, 0},
		{0, 6, 5, 6, 0},
		{0, 0, 6, 0, 0},
		{0, 0, 0, 0, 0}},

		{{0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0},
		{0, 0, 6, 0, 0},
		{0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0}},
	};
}