#pragma once

#include <glm/glm.hpp>

#include <vector>

class Chunk {
public:
	Chunk(int pX, int pY, int pZ, bool pEmpty);
	~Chunk();

	void addBlock(std::uint32_t pBlock);
	glm::vec3 getPosition();

	bool operator==(const Chunk& pChunk);
	bool operator!=(const Chunk& pChunk);

	void setIgnoreLeft(bool pIgnore);
	void setIgnoreRight(bool pIgnore);
	void setIgnoreDown(bool pIgnore);
	void setIgnoreUp(bool pIgnore);
	void setIgnoreFront(bool pIgnore);
	void setIgnoreBack(bool pIgnore);

	bool getIgnoreLeft();
	bool getIgnoreRight();
	bool getIgnoreDown();
	bool getIgnoreUp();
	bool getIgnoreFront();
	bool getIgnoreBack();

	bool isEmpty();
	std::vector<std::uint32_t>& getBlocks();

private:
	std::vector<std::uint32_t> blocks;
	glm::vec3 position;

	bool empty = true;

	bool ignoreLeft = false;
	bool ignoreRight = false;
	bool ignoreDown = false;
	bool ignoreUp = false;
	bool ignoreFront = false;
	bool ignoreBack = false;
};