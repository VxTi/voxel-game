//
// Created by Luca Warmenhoven on 20/05/2024.
//
#include "World.h"
#include "SimplexNoise.h"
#include "../rendering/model/Mesh.h"
#include <iostream>
#include <random>


void world_generation_fn(World *world, Transformation *observationPoint)
{

    if ( observationPoint == nullptr ) {
        std::cerr << "No observation point provided, aborting world generation." << std::endl;
        return;
    }

    // Generate chunks if the observation point is within range.
    if ( world->worldObjects.size() > 100)
        return;

    // Generate chunks around the observation point in a circular manner
    int32_t x, z, px, pz;
    const int chunk_generation_radius = 10;
    // Wait until the thread is stopped
    std::chrono::milliseconds interval(1);

    while ( true ) {
        if (world->chunks.size() > 1000)
            return;
        px = (((int32_t) observationPoint->position.x) / CHUNK_SIZE) * CHUNK_SIZE;
        pz = ((int32_t) observationPoint->position.z) / CHUNK_SIZE * CHUNK_SIZE;

        for ( x = -chunk_generation_radius; x < chunk_generation_radius; x++ ) {
            for ( z = -chunk_generation_radius; z < chunk_generation_radius; z++ ) {
                //std::this_thread::sleep_for(interval);
                world->generateChunk(
                        px + x * CHUNK_SIZE,
                        pz + z * CHUNK_SIZE
                );
            }
        }
    }
}

void World::startWorldGeneration(Transformation *observationPoint)
{

    // If it's already started then just ... just don't.
    if ( this->worldGenerationThread )
        return;


    drawables = std::vector<Drawable *>();
    worldObjects = std::vector<Updatable *>();

    worldGenerationThread = new std::thread(world_generation_fn, this, observationPoint);
}

void World::render(float deltaTime, Transformation *transformation, Frustum frustum)
{
    for ( chunk_t *chunk : chunks ) {
        chunk->mesh->draw(deltaTime);
    }
    for ( Drawable *drawable: drawables ) {

        // TODO: Implement
        /*if ( !isWithinFrustum(transformation, frustum, drawable->position)) {
            continue;
        }*/

        drawable->draw(0);
    }

    // If there's chunks that need their meshes to be generated, then do so.
    if ( !chunkMeshGenerationQueue.empty()) {
        immature_chunk_data_t *chunk_mesh_data = chunkMeshGenerationQueue.front();
        generateChunkMesh(chunk_mesh_data->chunk, chunk_mesh_data->mesh_data);
        chunkMeshGenerationQueue.pop();
    }
}

/**
 * Get the biome noise height for a specific coordinate.
 * This can be used to scale certain biomes.
 */
static float getBiomeNoise(float x, float z)
{
    return ( SimplexNoise::noise((float) x / 100.0f, (float) z / 100.0f) + 1 ) / 2;
}

/**
 * Get the height of the chunk at a certain coordinate.
 */
static float getChunkHeight(float x, float z)
{
    x /= 10.0f;
    z /= 10.0f;
    float biome_noise = getBiomeNoise(x, z);
    float biome_height_factor = World::BIOME_HEIGHT_SCALING_FACTORS[ (int) ( biome_noise * CHUNK_BIOME_COUNT) ];
    biome_height_factor *= biome_noise;
    float resultingHeight = 0.0f;

    for ( auto octave: World::CHUNK_GENERATION_OCTAVES ) {
        resultingHeight += SimplexNoise::noise( x / octave[ 0 ] ,  z / octave[ 0 ]) * octave[ 1 ];
    }
    resultingHeight = ( resultingHeight ) * CHUNK_GENERATION_MAX_HEIGHT;
    return biome_height_factor * resultingHeight;
}

/**
 * Get the normal vector at a specific location.
 * This calculates the normal vector based on the adjacent
 * height points, which are calculated by the `getChunkHeight` function.
 */
static vec3 getNormalVector(float x, float z)
{
    // Calculate the normal vectors
    float height1 = getChunkHeight(x - CHUNK_GENERATION_NORMAL_DELTA, z);
    float height2 = getChunkHeight(x + CHUNK_GENERATION_NORMAL_DELTA, z);
    float height3 = getChunkHeight(x, z - CHUNK_GENERATION_NORMAL_DELTA);
    float height4 = getChunkHeight(x, z + CHUNK_GENERATION_NORMAL_DELTA);

    return normalize(vec3(height1 - height2, 2.0f * CHUNK_GENERATION_NORMAL_DELTA, height3 - height4));
}

/*
 * Generate the mesh for a chunk.
 * This function will startWorldGeneration a mesh with the provided vertices and indices.
 */
void World::generateChunkMesh(chunk_t *chunk, vbo_data_t *vbo_data)
{
    VBO *mesh = new VBO();
    mesh->withVertices(vbo_data->vertices, vbo_data->vertices_count);
    mesh->withIndices(vbo_data->indices, vbo_data->indices_count);
    mesh->build();
    chunk->mesh = mesh;
    // Add chunk to world
    chunks.push_back(chunk);

    // Free old memory, it's been copied video memory.
    free(vbo_data->indices);
    free(vbo_data->vertices);
    free(vbo_data);
}

void World::update(float deltaTime)
{
    for ( Updatable *updatable: worldObjects ) {
        updatable->update(deltaTime);
    }
}

/*
 * Generate a chunk at the given coordinates.
 * This function will startWorldGeneration both the height-map coordinates for the chunk, and the mesh mesh_data.
 * This mesh mesh_data can then be fed into the GPU for rendering.
 */
void World::generateChunk(int32_t x, int32_t z)
{
    for ( chunk_t *chunk: chunks ) {
        if ( chunk->x == x && chunk->z == z ) {
            return;
        }
    }
    auto *data_points = (float *) malloc(sizeof(float) * CHUNK_SIZE * CHUNK_SIZE);

    int32_t chunk_x, chunk_z, i, j;

    float cx, cy, cz;

    // Memory for mesh
    int mesh_width = CHUNK_SIZE + 1;
    const int indices_count = CHUNK_SIZE * CHUNK_SIZE * 6;

    int top_left, bottom_left, top_right, bottom_right;

    int indices_index = 0;
    int vertices_index = 0;

    // Vector chunk_mesh_data that will be passed to the main thread.
    auto *indices = (unsigned int *) malloc(sizeof(unsigned int) * indices_count);
    auto *vertices = (vertex_t *) malloc(sizeof(vertex_t) * mesh_width * mesh_width);

    glm::vec3 normal;

    // Generate height chunk_mesh_data points
    for ( i = 0; i <= CHUNK_SIZE; i++ ) {
        for (j = 0; j <= CHUNK_SIZE; j++)
        {
            // Calculate height for chunk at specific coordinate
            chunk_x = x + i;
            chunk_z = z + j;
            cx = (float) chunk_x - 0.5f;
            cz = (float) chunk_z - 0.5f;
            cy = getChunkHeight(cx, cz);

            normal = getNormalVector(cx, cz);
            // Add regular vertex
            vertices[ vertices_index++ ] = {
                    cx , cy, cz,
                    normal.x, normal.y, normal.z
            };

            // Don't add indices for the last row and column

            if (i < CHUNK_SIZE && j < CHUNK_SIZE) {

                data_points[ i * CHUNK_SIZE + j ] = cy;

                top_left = i * mesh_width + j;
                bottom_left = ( i + 1 ) * mesh_width + j;
                top_right = i * mesh_width + ( j + 1 );
                bottom_right = ( i + 1 ) * mesh_width + ( j + 1 );

                indices[ indices_index++ ] = top_left;
                indices[ indices_index++ ] = top_right;
                indices[ indices_index++ ] = bottom_left;

                indices[ indices_index++ ] = top_right;
                indices[ indices_index++ ] = bottom_right;
                indices[ indices_index++ ] = bottom_left;
            }
        }
    }

    auto *generated = (chunk_t *) malloc(sizeof(chunk_t));
    generated->height_map = data_points;
    generated->x = x;
    generated->z = z;

    // Store chunk_mesh_data in destination. The memory can be freed
    // after the mesh has been generated.
    auto *chunk_mesh_data = (immature_chunk_data_t *) malloc(sizeof(immature_chunk_data_t));
    chunk_mesh_data->mesh_data = (vbo_data_t *) malloc(sizeof(vbo_data_t));
    chunk_mesh_data->mesh_data->indices = indices;
    chunk_mesh_data->mesh_data->vertices = vertices;
    chunk_mesh_data->mesh_data->indices_count = indices_count;
    chunk_mesh_data->mesh_data->vertices_count = mesh_width * mesh_width;
    chunk_mesh_data->chunk = generated;

    // Add to mesh generation queue
    chunkMeshGenerationQueue.push(chunk_mesh_data);
}

World::~World()
{
    // Delete content of queue
    immature_chunk_data_t *data;

    // Clear all memory from the queue
    while ( !chunkMeshGenerationQueue.empty()) {
        data = chunkMeshGenerationQueue.front();
        free(data->mesh_data->indices);
        free(data->mesh_data->vertices);
        free(data->mesh_data);
        free(data->chunk);
        chunkMeshGenerationQueue.pop();
    }

    for ( chunk_t *chunk: chunks ) {
        free(chunk->height_map);
        delete chunk->mesh;
        free(chunk);
    }
    drawables.clear();
    worldObjects.clear();
    // Stop the world generation thread
    if ( worldGenerationThread )
        worldGenerationThread->detach();

    delete worldGenerationThread;

}