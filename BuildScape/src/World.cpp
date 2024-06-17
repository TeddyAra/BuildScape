#include "World.h"

// Voxel indices for each direction
const unsigned int cubeIndicesLeft[] = {
	4, 6, 2,
	4, 2, 0
};

const unsigned int cubeIndicesRight[] = {
	1, 3, 7,
	1, 7, 5
};

const unsigned int cubeIndicesTop[] = {
	2, 6, 7,
	2, 7, 3
};

const unsigned int cubeIndicesBottom[] = {
	4, 0, 1,
	4, 1, 5
};

const unsigned int cubeIndicesFront[] = {
	0, 2, 3,
	0, 3, 1
};

const unsigned int cubeIndicesBack[] = {
	4, 6, 7,
	4, 7, 5
};

World::World(float pVoxelSize, int pTopLayer, Camera* pCamera, Renderer* pRenderer)
	: voxelSize(pVoxelSize), topLayer(pTopLayer), camera(pCamera), renderer(pRenderer), shaderProgram(NULL), wireframe(0)
{
	checkCurrentChunk = true;
	interalFacesCulled = false;
	closestChunkPos = glm::vec3(0, 0, 0);
}

World::~World() {
	delete camera;
	delete renderer;
}

void World::generate() {
	// Go through each chunk position
	for (int cZ = -6; cZ < 6; cZ++) {
		for (int cY = -4; cY < 5; cY++) {
			for (int cX = -6; cX < 6; cX++) {
				// Create a chunk
				Chunk chunk(cX * 16 * voxelSize, cY * 16 * voxelSize, cZ * 16 * voxelSize, true);

				// Only generate chunks in the middle for testing purposes
				if (cY == 0 && cX > -2 && cX < 3 && cZ > -2 && cZ < 3) {
					// Generate blocks for these chunks
					chunk.setEmpty(false);
					for (int y = 0; y < 16; y++) {
						for (int z = 0; z < 16; z++) {
							for (int x = 0; x < 16; x++) {
								std::uint32_t block = 0;

								// Position
								block |= (x << 28);
								block |= (y << 24);
								block |= (z << 20);

								// ID
								int air = 0;
								if (y < topLayer - 1) air = 1;
								if (y == topLayer - 1) air = Random::range(0, 1);

								block |= (air << 12);

								chunk.addBlock(block);
							}
						}
					}
				}

				chunks.push_back(chunk);
			}
		}
	}
}

void World::checkChunk() {
	// No longer check the chunks
	setCheckChunk(false);

	// Get the camera's position
	glm::vec3 pos = camera->getLocked() ? camera->getLockedPosition() : camera->getPosition();

	// Go through the chunks
	for (Chunk& chunk : chunks) {
		// If the camera is in that chunk
		if (pos.x > chunk.getPosition().x && pos.x < chunk.getPosition().x + 16 * voxelSize &&
			pos.y > chunk.getPosition().y && pos.y < chunk.getPosition().y + 16 * voxelSize &&
			pos.z > chunk.getPosition().z && pos.z < chunk.getPosition().z + 16 * voxelSize) {

			glm::vec3 newClosest = chunk.getPosition();

			// Check if it's not the current chunk
			if (newClosest != closestChunkPos) {
				closestChunkPos = newClosest;

				// Make sure current chunk can be seen entirely
				chunk.setIgnoreRight(false);
				chunk.setIgnoreLeft(false);
				chunk.setIgnoreUp(false);
				chunk.setIgnoreDown(false);
				chunk.setIgnoreFront(false);
				chunk.setIgnoreBack(false);

				// Go through all chunks
				for (Chunk& chunkCopy : chunks) {
					// Ignore current chunk and empty chunks
					if (chunkCopy.getPosition() == closestChunkPos || chunkCopy.isEmpty()) continue;

					// Have the renderer ignore some faces depending on where the chunk is
					chunkCopy.setIgnoreLeft(chunkCopy.getPosition().x < closestChunkPos.x);
					chunkCopy.setIgnoreRight(chunkCopy.getPosition().x > closestChunkPos.x);
					chunkCopy.setIgnoreDown(chunkCopy.getPosition().y < closestChunkPos.y);
					chunkCopy.setIgnoreUp(chunkCopy.getPosition().y > closestChunkPos.y);
					chunkCopy.setIgnoreBack(chunkCopy.getPosition().z > closestChunkPos.z);
					chunkCopy.setIgnoreFront(chunkCopy.getPosition().z < closestChunkPos.z);
				}
			}

			break;
		}
	}
}

void World::setCheckChunk(bool pCheck) {
	checkCurrentChunk = pCheck;
}

glm::vec3 World::getClosestChunkPosition() {
	return closestChunkPos;
}

std::vector<Chunk> World::getChunks() {
	return chunks;
}

void World::setShaderProgram(GLuint pShaderProgram) {
	shaderProgram = pShaderProgram;
}

void World::setWireframeColour(int pColour) {
	wireframe = pColour;
}

int World::getWireframeColour() {
	return wireframe;
}

void World::draw() {
	for (Chunk chunk : chunks) {
		// Ignore empty chunks
		if (chunk.isEmpty()) continue;

		for (const auto& block : chunk.getBlocks()) {
			// Ignore air blocks
			int id = (block >> 12) & 0xFF;
			if (id == 0) continue;

			int x = (block >> 28) & 0x0F;
			int y = (block >> 24) & 0x0F;
			int z = (block >> 20) & 0x0F;

			// Get the position of the block
			glm::vec3 pos(glm::vec3(x * voxelSize, y * voxelSize, z * voxelSize) + chunk.getPosition());

			// Check which faces to draw
			int left = 0;
			int right = 0;
			int down = 0;
			int up = 0;
			int front = 0;
			int back = 0;

			if (interalFacesCulled) {
				left = (block >> 11) & 0x01;
				right = (block >> 10) & 0x01;
				down = (block >> 9) & 0x01;
				up = (block >> 8) & 0x01;
				front = (block >> 7) & 0x01;
				back = (block >> 6) & 0x01;
			}

			// Ignore empty blocks
			if (left + right + down + up + front + back == 0) continue;

			// Set model uniform
			GLuint modelLoc = glGetUniformLocation(shaderProgram, "model");
			glm::mat4 model = glm::translate(glm::mat4(1.0f), pos);
			glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

			// Set render colour
			GLuint colLoc = glGetUniformLocation(shaderProgram, "col");
			glUniform3f(colLoc, wireframe, wireframe, wireframe);

			// Get all indices
			std::vector<GLuint> combinedIndices;

			if (left == 0 &&  !chunk.getIgnoreLeft())  combinedIndices.insert(combinedIndices.end(), std::begin(cubeIndicesLeft),   std::end(cubeIndicesLeft));
			if (right == 0 && !chunk.getIgnoreRight()) combinedIndices.insert(combinedIndices.end(), std::begin(cubeIndicesRight),  std::end(cubeIndicesRight));
			if (up == 0 &&    !chunk.getIgnoreUp())    combinedIndices.insert(combinedIndices.end(), std::begin(cubeIndicesTop),    std::end(cubeIndicesTop));
			if (down == 0 &&  !chunk.getIgnoreDown())  combinedIndices.insert(combinedIndices.end(), std::begin(cubeIndicesBottom), std::end(cubeIndicesBottom));
			if (front == 0 && !chunk.getIgnoreFront()) combinedIndices.insert(combinedIndices.end(), std::begin(cubeIndicesFront),  std::end(cubeIndicesFront));
			if (back == 0 &&  !chunk.getIgnoreBack())  combinedIndices.insert(combinedIndices.end(), std::begin(cubeIndicesBack),   std::end(cubeIndicesBack));

			// Bind the EBO and assign the indices
			renderer->bindEBO();
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, combinedIndices.size() * sizeof(GLuint), combinedIndices.data(), GL_STATIC_DRAW);

			// Draw the block's triangles and unbind the EBO
			glDrawElements(GL_TRIANGLES, combinedIndices.size(), GL_UNSIGNED_INT, 0);
			renderer->unbindEBO();
		}
	}
}

int World::isNeighbourPresent(const std::vector<std::uint32_t>& blocks, int index, int dir) {
	// Get the block's position
	std::uint32_t block = blocks[index];
	int x = (block >> 28) & 0x0F;
	int y = (block >> 24) & 0x0F;
	int z = (block >> 20) & 0x0F;

	// Update index depending on where to look
	switch (dir) {
	case 0: // Left
		if (x == 0) return 0;
		index -= 1;
		break;
	case 1: // Right
		if (x == 15) return 0;
		index += 1;
		break;
	case 2: // Down
		if (y == 0) return 0;
		index -= 256;
		break;
	case 3: // Up
		if (y == 15) return 0;
		index += 256;
		break;
	case 4: // Front
		if (z == 0) return 0;
		index -= 16;
		break;
	case 5: // Back
		if (z == 15) return 0;
		index += 16;
		break;
	}

	// Get the neighbouring block's id
	block = blocks[index];
	int id = (block >> 12) & 0xFF;
	return id == 0 ? 0 : 1;
}

void World::internalFaceCull() {
	interalFacesCulled = true;
	for (Chunk& chunk : chunks) {
		std::vector<std::uint32_t>& blocks = chunk.getBlocks();

		for (size_t i = 0; i < blocks.size(); ++i) {
			// Ignore air blocks
			std::uint32_t block = blocks[i];
			int id = (block >> 12) & 0xFF;
			if (id == 0) continue;

			// Make a copy of the block
			std::uint32_t blockCopy = block;

			// Check which faces to draw
			blockCopy &= ~(0x3F << 6);
			for (int j = 0; j < 6; ++j) {
				if (isNeighbourPresent(blocks, i, j)) {
					blockCopy |= (1 << (11 - j));
				}
			}

			// Update the block
			blocks[i] = blockCopy;
		}
	}
}

bool World::areInternalFacesCulled() {
	return interalFacesCulled;
}