#include "Layer.hpp"

double Layer::getValue(unsigned long long int index) const {
	return weights[index];
}

LayerValues Layer::getValues() const {
	LayerValues result;
	result.rows = num_output;
	result.cols = num_input;
	result.values = new double[num_weights];
	for (size_t i = 0; i < num_weights; i++) {
		result.values[i] = weights[i];
	}
	return result;
}

unsigned int Layer::getNumInput() const {
	return num_input;
}

unsigned int Layer::getNumOutput() const {
	return num_output;
}

unsigned int Layer::getRows() const {
	return num_output;
}

unsigned int Layer::getCols() const {
	return num_input;
}
