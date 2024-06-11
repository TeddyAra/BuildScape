#pragma once

#include <vector>

#include "Chunk.h"

class World {
public:
	World();
	~World();

	void Generate();


private:
	std::vector<Chunk> chunks;
};