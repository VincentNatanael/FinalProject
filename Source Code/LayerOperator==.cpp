#include "Layer.hpp"

bool Layer::operator==(const Layer& operand) const {
	if (weights == nullptr || operand.weights == nullptr) return weights == operand.weights;
	if (num_input != operand.num_input) return false;
	if (num_output != operand.num_output) return false;
	if (num_weights != operand.num_weights) return false;
	for (unsigned long long int i = 0; i < num_weights; i++) {
		if (weights[i] != operand.weights[i]) return false;
	}
	return true;
}