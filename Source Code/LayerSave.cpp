#include "Layer.hpp"

#include <fstream>

void Layer::save(std::string filename) const {
	if (weights == nullptr) throw "Weights are not initialized";
	if (biases == nullptr) throw "Biases are not initialized";

	std::ofstream file(filename, std::ios::binary);
	if (!file.is_open()) throw "Fail to open file";

	file.write((char*)&num_input, sizeof(unsigned int));
	file.write((char*)&num_output, sizeof(unsigned int));
	for (unsigned long long int i = 0; i < num_weights; i++) {
		file.write((char*)(weights + i), sizeof(double));
	}
	for (unsigned long long int i = 0; i < num_output; i++) {
		file.write((char*)(biases + i), sizeof(double));
	}
	file.close();
}