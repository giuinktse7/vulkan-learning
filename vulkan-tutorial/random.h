#pragma once

#include <stdint.h>
#include <random>
#include <cfloat>

/*
	See https://stackoverflow.com/a/16224552/7157626
*/
class Random
{
public:
	Random(uint32_t seed);
	Random();

	double nextDouble();
	int nextInt(int from, int to);

private:
	std::mt19937 randomEngine;

	std::uniform_real_distribution<double> distribution;

	void initialize(uint32_t seed);
};