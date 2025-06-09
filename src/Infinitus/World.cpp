#include "FastNoiseSIMD/FastNoiseSIMD.h"
#include "World.hpp"
#include <limits>
#include <algorithm>

Tetra::World::World() : current_player_chunk(0, 0, 0), last_player_chunk(0, 0, 0),
	populated(false), meshed(false)
{
	// Initialize threading arrays
        for(uint8_t i = 0; i < THREADS; ++i) {
            is_thread_busy[i] = false;
            was_thread_launched[i] = false;
        }
        
	// Initial chunk loading around spawn point
	// Force initial player chunk calculation
	glm::fvec3 player_pos = Oreginum::Camera::get_position();
	current_player_chunk = world_pos_to_chunk_pos(player_pos);
	last_player_chunk = current_player_chunk;
	
	// Load initial chunks around player (horizontal infinite, limited vertical)
	const int VERTICAL_LOAD_DISTANCE = 2; // Only 5 chunks vertically
	for(int x = current_player_chunk.x - LOAD_DISTANCE; x <= current_player_chunk.x + LOAD_DISTANCE; ++x) {
		for(int y = current_player_chunk.y - VERTICAL_LOAD_DISTANCE; y <= current_player_chunk.y + VERTICAL_LOAD_DISTANCE; ++y) {
			for(int z = current_player_chunk.z - LOAD_DISTANCE; z <= current_player_chunk.z + LOAD_DISTANCE; ++z) {
				glm::ivec3 chunk_pos(x, y, z);
				load_chunk(chunk_pos);
			}
		}
	}
	
	//Initial world creation
	while(!populated) update();
}

Tetra::World::~World()
{
	// Wait for all threads to finish
        for(uint8_t i = 0; i < THREADS; ++i) {
		if(was_thread_launched[i]) threads[i].join();
	}
	
	// Clean up deletion queue
	for(auto& d : deletion_queue) delete d.first;
	
	// Clean up all loaded chunks
	std::lock_guard<std::mutex> chunks_guard{chunks_mutex};
	for(auto& pair : loaded_chunks) {
            delete pair.second;
	}
	loaded_chunks.clear();
}

glm::ivec3 Tetra::World::world_pos_to_chunk_pos(const glm::fvec3& world_pos)
{
	return glm::ivec3(
		static_cast<int>(std::floor(world_pos.x / CHUNK_SIZE)),
		static_cast<int>(std::floor(world_pos.y / CHUNK_SIZE)),
		static_cast<int>(std::floor(world_pos.z / CHUNK_SIZE))
	);
}

bool Tetra::World::is_chunk_loaded(const glm::ivec3& chunk_pos)
{
	std::lock_guard<std::mutex> chunks_guard{chunks_mutex};
	return loaded_chunks.find(chunk_pos) != loaded_chunks.end();
}

Tetra::Chunk* Tetra::World::get_chunk_at(const glm::ivec3& chunk_pos)
{
	std::lock_guard<std::mutex> chunks_guard{chunks_mutex};
	auto it = loaded_chunks.find(chunk_pos);
	return (it != loaded_chunks.end()) ? it->second : nullptr;
}

bool Tetra::World::is_chunk_in_render_distance(const glm::ivec3& chunk_pos, const glm::ivec3& player_chunk)
{
	glm::ivec3 diff = abs(chunk_pos - player_chunk);
	const int VERTICAL_RENDER_DISTANCE = 2;
	return diff.x <= RENDER_DISTANCE && diff.y <= VERTICAL_RENDER_DISTANCE && diff.z <= RENDER_DISTANCE;
}

bool Tetra::World::is_chunk_in_load_distance(const glm::ivec3& chunk_pos, const glm::ivec3& player_chunk)
{
	glm::ivec3 diff = abs(chunk_pos - player_chunk);
	const int VERTICAL_LOAD_DISTANCE = 2;
	return diff.x <= LOAD_DISTANCE && diff.y <= VERTICAL_LOAD_DISTANCE && diff.z <= LOAD_DISTANCE;
}

void Tetra::World::load_chunk(const glm::ivec3& chunk_pos)
{
	if(is_chunk_loaded(chunk_pos)) return;
	
	// Create new chunk
	glm::fvec3 world_translation = glm::fvec3(chunk_pos) * static_cast<float>(CHUNK_SIZE);
	Tetra::Chunk* new_chunk = new Tetra::Chunk(world_translation, glm::fvec3(0), 
		glm::u8vec3(chunk_pos.x & 255, chunk_pos.y & 255, chunk_pos.z & 255));
	
	// Add to loaded chunks
	{
		std::lock_guard<std::mutex> chunks_guard{chunks_mutex};
		loaded_chunks[chunk_pos] = new_chunk;
	}
}

void Tetra::World::unload_chunk(const glm::ivec3& chunk_pos)
{
	Tetra::Chunk* chunk = get_chunk_at(chunk_pos);
	if(!chunk) return;
	
	// Remove render groups immediately
                chunk->remove_render_groups();
                
	// Add to deletion queue
	{
		std::lock_guard<std::mutex> deletion_queue_guard{deletion_queue_mutex};
		chunk->set_being_deleted(true);
		deletion_queue.emplace_back(chunk, 0);
	}
	
	// Remove from loaded chunks
	{
		std::lock_guard<std::mutex> chunks_guard{chunks_mutex};
		loaded_chunks.erase(chunk_pos);
	}
}

void Tetra::World::update_chunks_around_player()
{
	// Get player position from camera
	glm::fvec3 player_pos = Oreginum::Camera::get_position();
	current_player_chunk = world_pos_to_chunk_pos(player_pos);
	
	// Check if player moved to a different chunk
	if(current_player_chunk != last_player_chunk)
	{
		// Determine chunks to load (horizontal infinite, limited vertical like Minecraft)
		const int VERTICAL_LOAD_DISTANCE = 2; // Only 5 chunks vertically (2 above, current, 2 below)
		for(int x = current_player_chunk.x - LOAD_DISTANCE; x <= current_player_chunk.x + LOAD_DISTANCE; ++x) {
			for(int y = current_player_chunk.y - VERTICAL_LOAD_DISTANCE; y <= current_player_chunk.y + VERTICAL_LOAD_DISTANCE; ++y) {
				for(int z = current_player_chunk.z - LOAD_DISTANCE; z <= current_player_chunk.z + LOAD_DISTANCE; ++z) {
					glm::ivec3 chunk_pos(x, y, z);
					if(!is_chunk_loaded(chunk_pos)) {
						chunks_to_load.insert(chunk_pos);
					}
				}
			}
		}
		
		// Determine chunks to unload (outside LOAD_DISTANCE)
		std::vector<glm::ivec3> chunks_to_remove;
		{
			std::lock_guard<std::mutex> chunks_guard{chunks_mutex};
			for(const auto& pair : loaded_chunks) {
				if(!is_chunk_in_load_distance(pair.first, current_player_chunk)) {
					chunks_to_remove.push_back(pair.first);
				}
			}
		}
		
		// Queue chunks for unloading
		for(const glm::ivec3& chunk_pos : chunks_to_remove) {
			chunks_to_unload.insert(chunk_pos);
		}
		
		last_player_chunk = current_player_chunk;
	}
	
	// Process chunk loading (limit per frame to avoid hitches)
	const int MAX_LOADS_PER_FRAME = 2;
	int loads_this_frame = 0;
	auto load_it = chunks_to_load.begin();
	while(load_it != chunks_to_load.end() && loads_this_frame < MAX_LOADS_PER_FRAME) {
		load_chunk(*load_it);
		load_it = chunks_to_load.erase(load_it);
		++loads_this_frame;
	}
	
	// Process chunk unloading
	const int MAX_UNLOADS_PER_FRAME = 5;
	int unloads_this_frame = 0;
	auto unload_it = chunks_to_unload.begin();
	while(unload_it != chunks_to_unload.end() && unloads_this_frame < MAX_UNLOADS_PER_FRAME) {
		unload_chunk(*unload_it);
		unload_it = chunks_to_unload.erase(unload_it);
		++unloads_this_frame;
	}
}

void Tetra::World::population_pass_1(Tetra::Chunk *chunk, uint8_t thread_index)
{
	populate_chunk_pass_1(chunk);

	chunk->set_populated(0, true);
	chunk->set_being_created(false);

	is_thread_busy[thread_index] = false;
}

void Tetra::World::population_pass_2(Tetra::Chunk *chunk, uint8_t thread_index)
{
	populate_chunk_pass_2(chunk);

	// For infinite world, we only set this specific chunk as populated
	chunk->set_populated(1, true);
	chunk->set_being_created(false);

	is_thread_busy[thread_index] = false;
}

void Tetra::World::mesh_chunk(Tetra::Chunk *chunk, uint8_t thread_index)
{
	//Cull, and mesh
	cull_chunk(chunk);
	chunk->create_mesh();

	//Emplace in add queue
	{
		std::lock_guard<std::mutex> guard{add_queue_mutex};
		add_queue.emplace_back(chunk);
	}

	chunk->set_meshed(true);
	chunk->set_being_created(false);
	is_thread_busy[thread_index] = false;
}

Tetra::Chunk *Tetra::World::get_population_pass_1_chunk()
{
	std::lock_guard<std::mutex> chunks_guard{chunks_mutex};
	
	// Only process chunks in render distance
	for(const auto& pair : loaded_chunks) {
		if(is_chunk_in_render_distance(pair.first, current_player_chunk)) {
			Tetra::Chunk* chunk = pair.second;
			if(!chunk->is_populated(0) && !chunk->is_being_created()) {
				return chunk;
			}
		}
	}
	return nullptr;
}

Tetra::Chunk *Tetra::World::get_population_pass_2_chunk()
{
	std::lock_guard<std::mutex> chunks_guard{chunks_mutex};
	
	// Only process chunks in render distance
	for(const auto& pair : loaded_chunks) {
		if(is_chunk_in_render_distance(pair.first, current_player_chunk)) {
			Tetra::Chunk* chunk = pair.second;
			if(!chunk->is_populated(1) && !chunk->is_being_created() && chunk->is_populated(0)) {
				return chunk;
			}
		}
	}
	return nullptr;
}

Tetra::Chunk *Tetra::World::get_unmeshed_chunk()
{
	std::lock_guard<std::mutex> chunks_guard{chunks_mutex};
	
	// Get closest chunk that is unmeshed within render distance
	Tetra::Chunk *result = nullptr;
	float closest_distance = std::numeric_limits<float>::max();
	
	for(const auto& pair : loaded_chunks) {
		if(is_chunk_in_render_distance(pair.first, current_player_chunk)) {
			Tetra::Chunk* chunk = pair.second;
			if(!chunk->is_meshed() && !chunk->is_being_created() && 
			   chunk->is_populated(0) && chunk->is_populated(1)) {
				
				// Calculate distance from player
				glm::fvec3 chunk_center = glm::fvec3(pair.first) * static_cast<float>(CHUNK_SIZE) + 
					glm::fvec3(CHUNK_SIZE/2);
				glm::fvec3 player_pos = Oreginum::Camera::get_position();
				float distance = glm::length(chunk_center - player_pos);
				
				if(distance < closest_distance) {
					closest_distance = distance;
					result = chunk;
				}
			}
		}
	}
	
	return result;
}

int8_t Tetra::World::get_thread()
{
	//Get thread
	int8_t thread_index{-1};
	for(uint8_t j{}; j < THREADS; ++j) if(!is_thread_busy[j]) thread_index = j;
	if(thread_index == -1) return -1;

	//Check and prepare chunk
	is_thread_busy[thread_index] = true;
	if(was_thread_launched[thread_index]) threads[thread_index].join();
	else was_thread_launched[thread_index] = true;
	return thread_index;
}

void Tetra::World::update()
{
	// Update chunks around player first
	update_chunks_around_player();
	
	//Wait 3 frames to ensure buffers aren't in use, then delete enqueued chunks
	if(deletion_queue.size())
	{
		std::lock_guard<std::mutex> deletion_queue_guard{deletion_queue_mutex};
		std::lock_guard<std::mutex> render_guard{
			*Oreginum::Renderer_Core::get_render_mutex()};
		for(uint32_t i{}; i < deletion_queue.size(); ++i)
		{
			if(deletion_queue[i].second > 5)
			{
				if(!deletion_queue[i].first->is_being_created())
				{
					for(uint32_t j{}; j < add_queue.size(); ++j)
						if(add_queue[j] == deletion_queue[i].first)
							add_queue.erase(add_queue.begin()+j);
					delete deletion_queue[i].first;
					deletion_queue.erase(deletion_queue.begin()+i);
				}
			} else ++deletion_queue[i].second;
		}
	}

	//Create chunk render groups
	for(uint8_t i{}; i < CHUNKS_ADDED_PER_FRAME; ++i)
	{
		if(add_queue.size())
		{
			if(!add_queue.front()->is_being_deleted()) 
			{
				add_queue.front()->create_render_groups();
				add_queue.front()->add_render_groups();
			}
			add_queue.erase(add_queue.begin());
		} else break;
	}

	//First pass population
	Tetra::Chunk *population_pass_1_chunk{nullptr};
	while(true)
	{
		population_pass_1_chunk = get_population_pass_1_chunk();
		if(!population_pass_1_chunk) break;

		int8_t thread_index{get_thread()};
		if(thread_index == -1) break;

		population_pass_1_chunk->set_being_created(true);
		threads[thread_index] = std::thread{&World::population_pass_1,
			this, population_pass_1_chunk, thread_index};
	}

	//If first pass population has completed, do second population pass
	Tetra::Chunk *population_pass_2_chunk{nullptr};
	while(!population_pass_1_chunk)
	{
		population_pass_2_chunk = get_population_pass_2_chunk();
		if(!population_pass_2_chunk) break;

		int8_t thread_index{get_thread()};
		if(thread_index == -1) break;

		population_pass_2_chunk->set_being_created(true);
		threads[thread_index] = std::thread{&World::population_pass_2,
			this, population_pass_2_chunk, thread_index};
	}

	populated = !population_pass_1_chunk && !population_pass_2_chunk;

	//If population has completed, cull and mesh unmeshed
	//chunks, then add them to the add queue
	Tetra::Chunk *unmeshed_chunk{nullptr};
	while(populated)
	{
		unmeshed_chunk = get_unmeshed_chunk();
		if(!unmeshed_chunk) break;

		int8_t thread_index{get_thread()};
		if(thread_index == -1) break;

		unmeshed_chunk->set_being_created(true);
		threads[thread_index] = std::thread{&World::mesh_chunk,
			this, unmeshed_chunk, thread_index};
	}

	meshed = !unmeshed_chunk;
}

float *Tetra::World::simplex(const glm::ivec3& offset, const glm::ivec3& size,
	float frequency, uint32_t octaves, uint32_t seed)
{
	FastNoiseSIMD *generator{FastNoiseSIMD::NewFastNoiseSIMD(seed)};
	generator->SetFrequency(frequency);
	generator->SetFractalOctaves(octaves);
	return generator->GetSimplexFractalSet(offset.x,
		offset.y, offset.z, size.x, size.y, size.z);
}

void Tetra::World::populate_chunk_pass_1(Tetra::Chunk *chunk)
{
	// Get chunk position in world coordinates
	glm::ivec3 chunk_world_pos = world_pos_to_chunk_pos(chunk->get_translation());
	glm::ivec3 chunk_offset = chunk_world_pos * static_cast<int>(CHUNK_SIZE);
	
	const glm::ivec3 CHUNK_OFFSET{chunk_offset.x, chunk_offset.y, chunk_offset.z},
		OFFSET_2D{CHUNK_OFFSET.z, CHUNK_OFFSET.x, 0}, SIZE_2D{CHUNK_SIZE, CHUNK_SIZE, 1};
	constexpr uint8_t RISES_BASES_HEIGHT{20}, EARTH_RANGE{50}, MOUNTAINOUSNESS_RANGE{200};

	float *mountainousness_set{simplex(OFFSET_2D, SIZE_2D, .003f, 7, SEED)};

	float *earth_set{simplex(OFFSET_2D, SIZE_2D, .0005f, 1, SEED+1)};
	float *hills_set{simplex(OFFSET_2D, SIZE_2D, .01f, 2, SEED+2)};
	float *detail_set{simplex(OFFSET_2D, SIZE_2D, .01f, 1, SEED+3)};
	float *plateau_fill_set{simplex({CHUNK_OFFSET.z, CHUNK_OFFSET.x, CHUNK_OFFSET.y},
		{CHUNK_SIZE, CHUNK_SIZE, CHUNK_SIZE}, .002f, 7, SEED+4)};
	float *plateau_height_set{simplex(OFFSET_2D, SIZE_2D, .003f, 2, SEED+5)};
		
	//Create ground
	uint32_t noise_index_2d{}, noise_index_3d{};
	for(uint8_t z{}; z < CHUNK_SIZE; ++z)
		for(uint8_t x{}; x < CHUNK_SIZE; ++x)
		{
			for(uint8_t y{}; y < CHUNK_SIZE; ++y)
			{
				const float MOUNTAINOUSNESS{std::max(
					mountainousness_set[noise_index_2d]*
					MOUNTAINOUSNESS_RANGE, 0.f)};

				float VOXEL_Y{static_cast<float>(chunk_offset.y + y)};

				const float EARTH{earth_set[noise_index_2d]*EARTH_RANGE};
				const float HILLS{hills_set[noise_index_2d]*5*MOUNTAINOUSNESS/30};
				const float DETAIL{detail_set[noise_index_2d]*2};
				const float GROUND{EARTH+HILLS+DETAIL};

				const float PLATEAU_HEIGHT{EARTH+plateau_height_set[noise_index_2d]*
					MOUNTAINOUSNESS-RISES_BASES_HEIGHT+DETAIL};
				const float PLATEAU{VOXEL_Y > PLATEAU_HEIGHT ?
					plateau_fill_set[noise_index_3d]*(VOXEL_Y-PLATEAU_HEIGHT) : 0};

				if(VOXEL_Y > GROUND || PLATEAU > .1)
					chunk->set_voxel_material({x, y, z}, Materials::STONE);

				++noise_index_3d;
			}
			++noise_index_2d;
		}

	FastNoiseSIMD::FreeNoiseSet(plateau_height_set);
	FastNoiseSIMD::FreeNoiseSet(plateau_fill_set);
	FastNoiseSIMD::FreeNoiseSet(detail_set);
	FastNoiseSIMD::FreeNoiseSet(hills_set);
	FastNoiseSIMD::FreeNoiseSet(earth_set);
	FastNoiseSIMD::FreeNoiseSet(mountainousness_set);
}

bool Tetra::World::axis_bounds_check(glm::u8vec3 values, glm::u8vec3 minimum, glm::u8vec3 maximum)
{
	for(uint8_t axis{}; axis < 3; ++axis)
		if(values[axis] < minimum[axis] || values[axis] > maximum[axis]) return true;
	return false;
}

void Tetra::World::inter_chunk_set(Tetra::Chunk *chunk, glm::i16vec3 voxel_index, uint8_t material)
{
	//If outside current chunk
	if(axis_bounds_check(glm::u8vec3(voxel_index), glm::u8vec3{0}, glm::u8vec3{CHUNK_SIZE-1}))
	{
		// Calculate which chunk this voxel belongs to
		glm::ivec3 current_chunk_pos = world_pos_to_chunk_pos(chunk->get_translation());
		glm::ivec3 target_chunk_pos = current_chunk_pos;
		
		// Adjust chunk position and voxel index based on which boundary was crossed
		for(uint8_t axis = 0; axis < 3; ++axis)
		{
			if(voxel_index[axis] < 0) 
			{
				--target_chunk_pos[axis];
				voxel_index[axis] = static_cast<int16_t>(CHUNK_SIZE) + voxel_index[axis];
			}
			else if(voxel_index[axis] >= CHUNK_SIZE) 
			{
				++target_chunk_pos[axis];
				voxel_index[axis] = voxel_index[axis] - static_cast<int16_t>(CHUNK_SIZE);
			}
		}

		// Try to find the target chunk
		Tetra::Chunk* target_chunk = get_chunk_at(target_chunk_pos);
		if(target_chunk && !target_chunk->is_being_deleted())
		{
			target_chunk->set_voxel_material(glm::u8vec3(voxel_index), material);
		}
		// If chunk doesn't exist or is being deleted, we just ignore the voxel
		// This is acceptable for features like trees that span chunk boundaries
	}
	//If inside current chunk
	else 
	{
		chunk->set_voxel_material(glm::u8vec3(voxel_index), material);
	}
}

void Tetra::World::create_tree(Tetra::Chunk *chunk, glm::i16vec3 base_voxel_index)
{
	base_voxel_index -= glm::i16vec3{TREE_SIZE.x/2, 0, TREE_SIZE.z/2};

	for(int8_t y{}; y < TREE_SIZE.y; ++y)
		for(int8_t x{}; x < TREE_SIZE.x; ++x)
			for(int8_t z{}; z < TREE_SIZE.z; ++z)
			{
				glm::i16vec3 voxel_index{base_voxel_index};
				voxel_index += glm::i16vec3{x, -y, z}; // Flip Y to account for downward-increasing coordinates
				uint8_t material{TREE[y][z][x]};
				if(!material) continue;
				inter_chunk_set(chunk, voxel_index, material);
			}
}

void Tetra::World::populate_chunk_pass_2(Tetra::Chunk *chunk)
{
	const glm::ivec3 chunk_world_pos = chunk->get_translation();
	const glm::ivec3 CHUNK_OFFSET{chunk_world_pos.x, chunk_world_pos.y, chunk_world_pos.z};
	const glm::ivec3 OFFSET_2D{CHUNK_OFFSET.z, CHUNK_OFFSET.x, 0};
	const glm::ivec3 SIZE_2D{CHUNK_SIZE, CHUNK_SIZE, 1};

	float *tree_area_set{simplex(OFFSET_2D, SIZE_2D, .003f, 5, SEED+6)};

	uint32_t noise_index_2d{};
	
	// Process each column (x,z) in this chunk
	for(uint8_t voxel_z{}; voxel_z < CHUNK_SIZE; ++voxel_z)
	{
		for(uint8_t voxel_x{}; voxel_x < CHUNK_SIZE; ++voxel_x)
		{
			// Find the surface level for this column by scanning from top to bottom  
			// Since Y increases downward, Y=0 is "top" and Y=CHUNK_SIZE-1 is "bottom"
			int surface_y = -1;
			bool water_surface = false;
			
			// First pass: find surface level (scan from top Y=0 to bottom Y=CHUNK_SIZE-1)
			for(int voxel_y = 0; voxel_y < CHUNK_SIZE; ++voxel_y)
			{
				glm::u8vec3 voxel_index{voxel_x, static_cast<uint8_t>(voxel_y), voxel_z};
				uint8_t voxel_material = chunk->get_voxel_material(voxel_index);
				float world_y = static_cast<float>(chunk_world_pos.y + voxel_y);
				
				// If we find solid material (stone from pass 1), this is our surface
				if(voxel_material == Materials::STONE)
				{
					surface_y = voxel_y;
					break;
				}
				// If we're below water level and no solid found yet, check for water placement  
				else if(world_y >= -10 && world_y <= -5 && voxel_material == 0 && surface_y == -1)
				{
					// This could be water surface
					water_surface = true;
					surface_y = voxel_y;
				}
			}
			
			// Second pass: assign materials based on distance from surface
			for(uint8_t voxel_y{}; voxel_y < CHUNK_SIZE; ++voxel_y)
			{
				glm::u8vec3 voxel_index{voxel_x, voxel_y, voxel_z};
				uint8_t voxel_material = chunk->get_voxel_material(voxel_index);
				float world_y = static_cast<float>(chunk_world_pos.y + voxel_y);
				
				// Calculate depth from surface
				int depth_from_surface = surface_y >= 0 ? static_cast<int>(voxel_y) - surface_y : -999;
				
				// Water placement - only in empty spaces that are below water level
				if(world_y >= -10 && world_y <= -5 && voxel_material == 0)
				{
					chunk->set_voxel_material(voxel_index, Materials::WATER);
					voxel_material = Materials::WATER;
				}
				// Sand near water level - only replace stone that's near water
				else if(world_y >= -10 && world_y <= -3 && voxel_material == Materials::STONE && surface_y >= 0 && depth_from_surface >= -2 && depth_from_surface <= 2)
				{
					chunk->set_voxel_material(voxel_index, Materials::SAND);
				}
				// Surface and subsurface materials
				else if(voxel_material == Materials::STONE && surface_y >= 0)
				{
					if(depth_from_surface == 0) // Surface
					{
						chunk->set_voxel_material(voxel_index, Materials::GRASS);
						
						// Only place trees on actual surface blocks
						float random = static_cast<float>(rand() % 2000) / 100.0f;
						const bool TREE = random < std::max(tree_area_set[noise_index_2d], 0.0f);
						if(TREE)
						{
							// Place tree base at the grass block
							glm::i16vec3 tree_base = glm::i16vec3(voxel_index);
							create_tree(chunk, tree_base);
						}
					}
					else if(depth_from_surface > 0 && depth_from_surface <= 4) // Below surface (dirt layer)
					{
						chunk->set_voxel_material(voxel_index, Materials::DIRT);
					}
					// Stone remains stone for deeper areas
				}
			}
			
			++noise_index_2d;
		}
	}

	FastNoiseSIMD::FreeNoiseSet(tree_area_set);
}

void Tetra::World::transparent_neighbor_cull(Tetra::Chunk *chunk, const glm::u8vec3& voxel_position)
{
	constexpr glm::i8vec3 NEIGHBORS[CUBE_FACES]{{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, 
		{0, -1, 0}, {0, 0, 1}, {0, 0, -1}};

	for(uint8_t face{}; face < CUBE_FACES; ++face)
	{
		glm::i16vec3 neighbor_position{voxel_position.x+NEIGHBORS[face].x,
			voxel_position.y+NEIGHBORS[face].y, voxel_position.z+NEIGHBORS[face].z};

		bool is_transparent = true;

		// Check if neighbor is within current chunk
		if(neighbor_position.x >= 0 && neighbor_position.x < CHUNK_SIZE &&
		   neighbor_position.y >= 0 && neighbor_position.y < CHUNK_SIZE &&
		   neighbor_position.z >= 0 && neighbor_position.z < CHUNK_SIZE)
		{
			is_transparent = chunk->is_voxel_transparent(glm::u8vec3(neighbor_position));
		}
		else 
		{
			// Neighbor is in a different chunk - try to find it
			glm::ivec3 current_chunk_pos = world_pos_to_chunk_pos(chunk->get_translation());
			glm::ivec3 neighbor_chunk_pos = current_chunk_pos;
			glm::i16vec3 neighbor_voxel = neighbor_position;
			
			// Adjust chunk position and voxel position for cross-chunk access
			for(uint8_t axis = 0; axis < 3; ++axis)
			{
				if(neighbor_voxel[axis] < 0) 
				{
					--neighbor_chunk_pos[axis];
					neighbor_voxel[axis] = static_cast<int16_t>(CHUNK_SIZE) + neighbor_voxel[axis];
				}
				else if(neighbor_voxel[axis] >= CHUNK_SIZE) 
				{
					++neighbor_chunk_pos[axis];
					neighbor_voxel[axis] = neighbor_voxel[axis] - static_cast<int16_t>(CHUNK_SIZE);
				}
			}

			// Try to get the neighboring chunk
			Tetra::Chunk* neighbor_chunk = get_chunk_at(neighbor_chunk_pos);
			if(neighbor_chunk && !neighbor_chunk->is_being_deleted())
			{
				is_transparent = neighbor_chunk->is_voxel_transparent(glm::u8vec3(neighbor_voxel));
			}
			else
			{
				// If neighboring chunk doesn't exist, assume it's air (transparent)
				is_transparent = true;
			}
		}

		// Set culling based on transparency
		chunk->set_voxel_culled(voxel_position, !is_transparent, face);
	}
}

void Tetra::World::cull_chunk(Tetra::Chunk *chunk)
{
	for(uint8_t z{}; z < CHUNK_SIZE; ++z)
		for(uint8_t y{}; y < CHUNK_SIZE; ++y)
			for(uint8_t x{}; x < CHUNK_SIZE; ++x)
			{
				glm::u8vec3 voxel_position{x, y, z};
				if(!chunk->is_voxel_transparent(voxel_position))
					transparent_neighbor_cull(chunk, voxel_position);
    }
} 