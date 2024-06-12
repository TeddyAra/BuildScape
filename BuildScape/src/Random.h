#pragma once

#include <random>

class Random {
public:
	Random() = delete;

	static int range(int pMin, int pMax);

private:
	static std::random_device rd;
	static std::mt19937 gen;
};