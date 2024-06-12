#include "Random.h"

std::random_device Random::rd;
std::mt19937 Random::gen(rd());

int Random::range(int pMin, int pMax) {
	std::uniform_int_distribution<> num(pMin, pMax);
	return (int)num(gen);
}