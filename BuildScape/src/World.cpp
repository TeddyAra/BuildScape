#include "World.h"

World::World(float pVoxelSize, int pTopLayer, Camera* pCamera, Renderer* pRenderer)
	: voxelSize(pVoxelSize), topLayer(pTopLayer), camera(pCamera), renderer(pRenderer), shaderProgram(NULL), wireframe(0), currentId(1)
{
	checkCurrentChunk = true;
	internalFacesCulled = false;
	closestChunkPos = glm::vec3(0, 0, 0);
}

World::~World() {

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
								if (y < topLayer) air = 1;

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

void World::clear() {
	internalFacesCulled = false;
	chunks.clear();
}

void World::checkChunk(bool pIgnoreIfCurrentChunk) {

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
			if (newClosest != closestChunkPos || pIgnoreIfCurrentChunk) {
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
					//chunkCopy.setIgnoreLeft(chunkCopy.getPosition().x < closestChunkPos.x);
					//chunkCopy.setIgnoreRight(chunkCopy.getPosition().x > closestChunkPos.x);
					//chunkCopy.setIgnoreDown(chunkCopy.getPosition().y < closestChunkPos.y);
					//chunkCopy.setIgnoreUp(chunkCopy.getPosition().y > closestChunkPos.y);
					//chunkCopy.setIgnoreBack(chunkCopy.getPosition().z > closestChunkPos.z);
					//chunkCopy.setIgnoreFront(chunkCopy.getPosition().z < closestChunkPos.z);

					//refreshChunk(chunkCopy);
				}
			}

			break;
		}
	}
}

void World::enableAllFaces() {
	for (Chunk& chunk : chunks) {
		chunk.setIgnoreRight(false);
		chunk.setIgnoreLeft(false);
		chunk.setIgnoreUp(false);
		chunk.setIgnoreDown(false);
		chunk.setIgnoreFront(false);
		chunk.setIgnoreBack(false);
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

void World::checkBlockManipulation() {
	bool destroy = Input::getMouseDown(0);
	bool place = false;
	if (!destroy) {
		place = Input::getMouseDown(1);
		if (!place) return;
	}

	Camera::IntersectionInfo finalInfo;
	float distance = std::numeric_limits<float>::max();
	int blockIt = -1;
	int chunkIt = -1;

	for (size_t i = 0; i < chunks.size(); ++i) {
		Chunk chunk = chunks[i];
		if (chunk.isEmpty()) continue;

		glm::vec3 chunkPos = chunk.getPosition();
		for (size_t j = 0; j < chunk.getBlocks().size(); ++j) {
			std::uint32_t& block = chunk.getBlocks()[j];

			int id = (block >> 12) & 0xFF;
			if (id == 0) continue;

			int x = (block >> 28) & 0x0F;
			int y = (block >> 24) & 0x0F;
			int z = (block >> 20) & 0x0F;

			glm::vec3 blockPos(x, y, z);
			glm::vec3 globalPos = chunkPos + blockPos * voxelSize;

			Camera::IntersectionInfo info = camera->checkIntersection(voxelSize, globalPos, block);

			if (info.intersected) {
				if (info.distance < distance) {
					finalInfo = info;
					distance = info.distance;

					chunkIt = i;
					blockIt = j;
				}
			}

			if (info.inside) {
				if (place) return;

				blockIt = j;
				chunkIt = i;
				goto endloops;
			}
		}
	}

endloops:

	if (blockIt == -1 || chunkIt == -1) return;

	if (destroy) {
		Chunk& chunk = chunks[chunkIt];
		std::uint32_t& block = chunk.getBlocks()[blockIt];

		block &= ~(0xFF << 12);

		cullChunk(chunk);
		refreshChunk(chunk);

		std::cout << "Destroyed" << std::endl;
	} else {
		Chunk& chunk = chunks[chunkIt];
		std::uint32_t& block = chunk.getBlocks()[blockIt];
		int dir = finalInfo.direction;
		int index = blockIt;

		int x = (block >> 28) & 0x0F;
		int y = (block >> 24) & 0x0F;
		int z = (block >> 20) & 0x0F;

		switch (dir) {
		case 3: // Left
			if (x == 0);
			index -= 1;
			break;
		case 2: // Right
			if (x == 15);
			index += 1;
			break;
		case 1: // Down
			if (y == 0);
			index -= 256;
			break;
		case 0: // Up
			if (y == 15);
			index += 256;
			break;
		case 5: // Back
			if (z == 0);
			index -= 16;
			break;
		case 4: // Front
			if (z == 15);
			index += 16;
			break;
		}

		std::uint32_t& newBlock = chunk.getBlocks()[index];
		newBlock |= (currentId << 12);

		cullChunk(chunk);
		refreshChunk(chunk);

		std::cout << "Placed with ID " << currentId << std::endl;
	}
}

void World::draw(glm::vec3 pSkyCol) {
	for (int i = 1; i < 5; i++) {
		if (Input::getKeyDown(48 + i)) {
			currentId = i;
			break;
		}
	}

	for (Chunk& chunk : chunks) {
		// Ignore empty chunks
		if (chunk.isEmpty()) continue;

		// Set model uniform
		GLuint modelLoc = glGetUniformLocation(shaderProgram, "model");
		glm::mat4 model = glm::translate(glm::mat4(1.0f), chunk.getPosition() * voxelSize);
		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

		renderer->setTextureUniforms(shaderProgram);

		GLuint lightLoc = glGetUniformLocation(shaderProgram, "lightDir");
		glm::vec3 lightDir = glm::normalize(glm::vec3(1, -5, -2));
		glUniform3f(lightLoc, lightDir.x, lightDir.y, lightDir.z);

		GLuint camLoc = glGetUniformLocation(shaderProgram, "camPos");
		glm::vec3 camPos = camera->getPosition();
		glUniform3f(camLoc, camPos.x, camPos.y, camPos.z);

		GLuint fogLoc = glGetUniformLocation(shaderProgram, "fogDis");
		glUniform2f(fogLoc, 50.0f, 20.0f);

		GLuint skyLoc = glGetUniformLocation(shaderProgram, "skyCol");
		glUniform3f(skyLoc, pSkyCol.x, pSkyCol.y, pSkyCol.z);

		if (checkCurrentChunk) {
			refreshChunk(chunk);
		}

		if (!chunk.instances.empty()) {
			renderer->bindInstanceVBO();
			glBufferData(GL_ARRAY_BUFFER, sizeof(Chunk::InstanceData) * chunk.instances.size(), chunk.instances.data(), GL_STATIC_DRAW);
			renderer->unbindInstanceVBO();

			renderer->bindEBO();
			glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0, chunk.instances.size());
			renderer->unbindEBO();
		} else {
			std::cerr << "[WARNING] No instances to draw for this chunk." << std::endl;
		}
	}

	checkCurrentChunk = false;
}

void World::refreshChunk(Chunk& pChunk) {
	pChunk.instances.clear();

	for (const auto& block : pChunk.getBlocks()) {
		// Ignore air blocks
		int id = (block >> 12) & 0xFF;
		if (id == 0) continue;

		int x = (block >> 28) & 0x0F;
		int y = (block >> 24) & 0x0F;
		int z = (block >> 20) & 0x0F;

		// Get the position of the block
		glm::vec3 pos(glm::vec3(x * voxelSize, y * voxelSize, z * voxelSize));

		// Check which faces to draw
		int left = (block >> 11) & 0x01;
		int right = (block >> 10) & 0x01;
		int down = (block >> 9) & 0x01;
		int up = (block >> 8) & 0x01;
		int front = (block >> 7) & 0x01;
		int back = (block >> 6) & 0x01;

		if (left == 0 && !pChunk.getIgnoreLeft())  pChunk.instances.push_back(addInstance(pos, 90, glm::vec3(0, 1, 0), id));
		if (right == 0 && !pChunk.getIgnoreRight()) pChunk.instances.push_back(addInstance(pos, 270, glm::vec3(0, 1, 0), id));
		if (down == 0 && !pChunk.getIgnoreDown())  pChunk.instances.push_back(addInstance(pos, 270, glm::vec3(1, 0, 0), id));
		if (up == 0 && !pChunk.getIgnoreUp())    pChunk.instances.push_back(addInstance(pos, 90, glm::vec3(1, 0, 0), id));
		if (back == 0 && !pChunk.getIgnoreBack())  pChunk.instances.push_back(addInstance(pos, 180, glm::vec3(0, 1, 0), id));
		if (front == 0 && !pChunk.getIgnoreFront()) pChunk.instances.push_back(addInstance(pos, 0, glm::vec3(0, 1, 0), id));
	}
}

Chunk::InstanceData World::addInstance(glm::vec3 pPosition, float pAngle, glm::vec3 pAxis, int pId) {
	Chunk::InstanceData instance;
	instance.offset = pPosition;

	glm::mat4 rotation = glm::mat4(1.0f);
	rotation = glm::rotate(rotation, glm::radians(pAngle), pAxis);
	instance.rotation = rotation;

	instance.id = (float)pId;

	return instance;
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
	internalFacesCulled = true;
	for (Chunk& chunk : chunks) {
		cullChunk(chunk);
	}
}

void World::cullChunk(Chunk& pChunk) {
	std::vector<std::uint32_t>& blocks = pChunk.getBlocks();

	for (size_t i = 0; i < blocks.size(); ++i) {
		// Ignore air blocks
		std::uint32_t block = blocks[i];
		int id = (block >> 12) & 0xFF;
		if (id == 0) {
			std::uint32_t blockCopy = block;

			blockCopy &= ~(0x3F << 6);
			continue;
		}

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

bool World::areInternalFacesCulled() {
	return internalFacesCulled;
}