#include "Layer.hpp"

#include <fstream>

void Layer::load(std::string filename) {
	if (weights != nullptr) delete[] weights;
	weights = nullptr;
	if (biases != nullptr) delete[] biases;
	weights = nullptr;
	num_input = 0;
	num_output = 0;
	num_weights = 0;

	std::ifstream file(filename, std::ios::binary);
	if (!file.is_open()) throw "Fail to open file";

	unsigned int* size = new unsigned int[2]{ 0,0 };
	file.read((char*)(size + 0), sizeof(unsigned int));
	file.read((char*)(size + 1), sizeof(unsigned int));
	if (file.fail()) throw "Invalid file";

	num_input = size[0];
	num_output = size[1];
	num_weights = (unsigned long long int)num_input * (unsigned long long int)num_output;
	weights = new double[num_weights];
	biases = new double[num_output];

	double* temp = new double(0);
	unsigned long long int i = 0;

	for (i = 0; i < num_weights; i++) {
		file.read((char*)temp, sizeof(double));
		weights[i] = *temp;
	}
	for (i = 0; i < num_output; i++) {
		file.read((char*)temp, sizeof(double));
		biases[i] = *temp;
	}

	/*
	for (i = 0; i < num_weights && !file.eof(); i++) {
		weights[i] = *temp;
		file.read((char*)temp, sizeof(double));
	}

	initWeights(i);
	*/

	delete temp;
	delete[] size;
	file.close();
}