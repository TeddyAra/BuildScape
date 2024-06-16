#pragma once

#include <vector>

#include "Chunk.h"
#include "Camera.h"
#include "Renderer.h"
#include "Random.h"
#include "GL/glew.h"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

class World {
public:
	World(float pVoxelSize, int pTopLayer, Camera* pCamera, Renderer* pRenderer);
	~World();

	void generate();
	void checkChunk();
	void setCheckChunk(bool pCheck);
	glm::vec3 getClosestChunkPosition();
	std::vector<Chunk> getChunks();
	void setShaderProgram(GLuint pShaderProgram);

	void internalFaceCull();
	bool areInternalFacesCulled();

	void setWireframeColour(int pColour);
	int getWireframeColour();
	void draw();

private:
	std::vector<Chunk> chunks;
	bool checkCurrentChunk;
	glm::vec3 closestChunkPos;
	float voxelSize;
	int topLayer;
	bool interalFacesCulled;
	int wireframe;

	Camera* camera;
	Renderer* renderer;
	GLuint shaderProgram;

	int isNeighbourPresent(const std::vector<std::uint32_t>& blocks, int index, int dir);
};