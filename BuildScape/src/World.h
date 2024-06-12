#pragma once

#include <vector>

#include "Chunk.h"
#include "Camera.h"

class World {
public:
	World(float pVoxelSize, int pTopLayer, Camera* pCamera);
	~World();

	void generate();
	void checkChunk();
	void setCheckChunk(bool pCheck);
	glm::vec3 getClosestChunkPosition();
	std::vector<Chunk> getChunks();

	void internalFaceCull();
	bool areInternalFacesCulled();

private:
	std::vector<Chunk> chunks;
	bool checkCurrentChunk;
	Camera* camera;
	glm::vec3 closestChunkPos;
	float voxelSize;
	int topLayer;
	bool interalFacesCulled;

	int isNeighbourPresent(const std::vector<std::uint32_t>& blocks, int index, int dir);
};