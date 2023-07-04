#include "Layer.hpp"

#include <chrono>
#include <random>

namespace {
	static unsigned int seed = std::chrono::system_clock::now().time_since_epoch().count();
	static std::default_random_engine engine(seed);
	static std::normal_distribution<double> randNormal(0, 1);
}

void Layer::initWeights(unsigned long long int index) {
	if (weights == nullptr) return;
	
	for (unsigned long long int i = index; i < num_weights; i++) {
		weights[i] = randNormal(engine);
	}

	for (unsigned long long int i = 0; i < num_output; i++) {
		biases[i] = randNormal(engine);
	}
}