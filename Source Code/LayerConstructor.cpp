#include "Layer.hpp"

Layer::Layer() {
	num_input = 0;
	num_output = 0;
	num_weights = 0;
	weights = nullptr;
	biases = nullptr;
}

Layer::Layer(unsigned int num_input_nodes, unsigned int num_output_nodes) :
	num_input(num_input_nodes), num_output(num_output_nodes),
	num_weights((unsigned long long int)num_input_nodes* (unsigned long long int)num_output_nodes)
{
	weights = new double[num_weights];
	biases = new double[num_output];
	initWeights();
}

Layer::Layer(const char* filename) {
	load(filename);
}

Layer::~Layer() {
	delete[] weights;
	delete[] biases;
	num_input = 0;
	num_output = 0;
	num_weights = 0;
}