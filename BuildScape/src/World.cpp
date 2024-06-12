#include "World.h"

#include "Random.h"

World::World(float pVoxelSize, int pTopLayer, Camera* pCamera)
	: voxelSize(pVoxelSize), topLayer(pTopLayer), camera(pCamera)
{
	checkCurrentChunk = true;
	interalFacesCulled = false;
	closestChunkPos = glm::vec3(0, 0, 0);
}

World::~World() {
	
}

void World::generate() {
	for (int cZ = -6; cZ < 6; cZ++) {
		for (int cY = -4; cY < 5; cY++) {
			for (int cX = -6; cX < 6; cX++) {
				Chunk chunk(cX * 16 * voxelSize, cY * 16 * voxelSize, cZ * 16 * voxelSize, true);

				if (cY == 0 && cX > -2 && cX < 3 && cZ > -2 && cZ < 3) {
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

					//if (INTERNAL_FACE_CULLING) internalFaceCulling(chunk);
				}

				chunks.push_back(chunk);
			}
		}
	}
}

void World::checkChunk() {
	setCheckChunk(false);
	glm::vec3 pos = camera->getLocked() ? camera->getLockedPosition() : camera->getPosition();

	for (Chunk& chunk : chunks) {
		if (pos.x > chunk.getPosition().x && pos.x < chunk.getPosition().x + 16 * voxelSize &&
			pos.y > chunk.getPosition().y && pos.y < chunk.getPosition().y + 16 * voxelSize &&
			pos.z > chunk.getPosition().z && pos.z < chunk.getPosition().z + 16 * voxelSize) {

			glm::vec3 newClosest = chunk.getPosition();

			if (newClosest != closestChunkPos) {
				closestChunkPos = newClosest;

				chunk.setIgnoreRight(false);
				chunk.setIgnoreLeft(false);
				chunk.setIgnoreUp(false);
				chunk.setIgnoreDown(false);
				chunk.setIgnoreFront(false);
				chunk.setIgnoreBack(false);

				for (Chunk& chunkCopy : chunks) {
					if (chunk.getPosition() == closestChunkPos || chunkCopy.isEmpty()) continue;

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

int World::isNeighbourPresent(const std::vector<std::uint32_t>& blocks, int index, int dir) {
	std::uint32_t block = blocks[index];
	int x = (block >> 28) & 0x0F;
	int y = (block >> 24) & 0x0F;
	int z = (block >> 20) & 0x0F;

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

	block = blocks[index];
	int id = (block >> 12) & 0xFF;
	return id == 0 ? 0 : 1;
}

void World::internalFaceCull() {
	interalFacesCulled = true;
	for (Chunk& chunk : chunks) {
		std::vector<std::uint32_t>& blocks = chunk.getBlocks();

		for (size_t i = 0; i < blocks.size(); ++i) {
			std::uint32_t block = blocks[i];
			int id = (block >> 12) & 0xFF;
			if (id == 0) continue;

			std::uint32_t blockCopy = block;

			blockCopy &= ~(0x3F << 6);
			for (int j = 0; j < 6; ++j) {
				if (isNeighbourPresent(blocks, i, j)) {
					blockCopy |= (1 << (11 - j));
				}
			}

			blocks[i] = blockCopy;
		}
	}
}

bool World::areInternalFacesCulled() {
	return interalFacesCulled;
}