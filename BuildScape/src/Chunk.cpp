#include "Chunk.h"

Chunk::Chunk(int pX, int pY, int pZ, bool pEmpty) 
	: position(glm::vec3(pX, pY, pZ)), empty(pEmpty) 
{

}

Chunk::~Chunk() {
	
}

void Chunk::addBlock(std::uint32_t pBlock) {
	blocks.push_back(pBlock);
}

glm::vec3 Chunk::getPosition() {
	return position;
}

bool Chunk::operator==(const Chunk& pChunk) {
	return (position == pChunk.position);
}

bool Chunk::operator!=(const Chunk& pChunk) {
	return (position != pChunk.position);
}

void Chunk::setIgnoreLeft(bool pIgnore) {
	ignoreLeft = pIgnore;
}

void Chunk::setIgnoreRight(bool pIgnore) {
	ignoreRight = pIgnore;
}

void Chunk::setIgnoreDown(bool pIgnore) {
	ignoreDown = pIgnore;
}

void Chunk::setIgnoreUp(bool pIgnore) {
	ignoreUp = pIgnore;
}

void Chunk::setIgnoreFront(bool pIgnore) {
	ignoreFront = pIgnore;
}

void Chunk::setIgnoreBack(bool pIgnore) {
	ignoreBack = pIgnore;
}

bool Chunk::getIgnoreLeft() {
	return ignoreLeft;
}

bool Chunk::getIgnoreRight() {
	return ignoreRight;
}

bool Chunk::getIgnoreDown() {
	return ignoreDown;
}

bool Chunk::getIgnoreUp() {
	return ignoreUp;
}

bool Chunk::getIgnoreFront() {
	return ignoreFront;
}

bool Chunk::getIgnoreBack() {
	return ignoreBack;
}

bool Chunk::isEmpty() {
	return empty;
}

std::vector<std::uint32_t>& Chunk::getBlocks() {
	return blocks;
}