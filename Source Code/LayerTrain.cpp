#include "Layer.hpp"

#include "CLWrapper.hpp"
#include <iostream>
#include <cmath>

namespace {
	double calculate_loss_mse(LayerValues output, LayerValues expected) {
		if (output.rows != expected.rows) throw "Invalid expected output vector size";
		if (output.cols != expected.cols) throw "Invalid expected output vector size";

		size_t size = (size_t)output.rows * (size_t)output.cols;

		double* loss = new double[size];
		double loss_batch = 0;

		for (unsigned int i = 0; i < size; i++) {
			loss[i] = output.values[i] - expected.values[i];
			loss[i] *= loss[i];
			loss_batch += loss[i];
		}

		return loss_batch / output.cols;
	}
	double relu_derivative(double value) {
		if (value >= 0) return 1;
		else return 0.5;
	}
	double tanh_derivative(double value) {
		return (1 - value * value) * 0.1;
	}
	double activation_derivative(double value, unsigned int activation_function = 0) {
		if (activation_function == 0) return tanh_derivative(value);
		return relu_derivative(value);
	}
}

double trainLayer(
	const Layer* layers, unsigned int layer_num,
	const LayerValues* training_data, const LayerValues* training_label, unsigned int training_num,
	bool update_layers, bool label_is_derivative, const double& loss
) {
	LayerValues* dummy = nullptr;
	double return_value = trainLayer(dummy, layers, layer_num, training_data, training_label, training_num, update_layers, label_is_derivative, loss);
	delete dummy;
	return return_value;
}

double trainLayer(
	LayerValues*& input_derivative_output,
	const Layer* layers, unsigned int layer_num,
	const LayerValues* training_data, const LayerValues* training_label, unsigned int training_num,
	bool update_layers, bool label_is_derivative, const double& loss
) {
	// Initialize Weight Updates
	double** weights_d = new double* [layer_num];
	for (unsigned int i = 0; i < layer_num; i++) {
		weights_d[i] = new double[layers[i].num_weights];
		for (unsigned int v = 0; v < layers[i].num_weights; v++) {
			weights_d[i][v] = 0;
		}
	}

	// Initialize Bias Updates
	double** biases_d = new double* [layer_num];
	for (unsigned int i = 0; i < layer_num; i++) {
		biases_d[i] = new double[layers[i].num_output];
		for (unsigned int v = 0; v < layers[i].num_output; v++) {
			biases_d[i][v] = 0;
		}
	}

	// Initialize Input Derivatives
	LayerValues* input_d = new LayerValues[training_num];

	// Initialize Loss for Update Scaling
	double loss_mag = 0;

	// For every training data,
	for (unsigned int tr_data = 0; tr_data < training_num; tr_data++) {
		// Copy Training Data to Input Vector
		LayerValues input;
		if (true) {
			input.rows = training_data[tr_data].rows;
			input.cols = training_data[tr_data].cols;
			size_t size = (size_t)input.rows * (size_t)input.cols;
			input.values = new double[size];

			CLArg arg1{ training_data[tr_data].values, size * sizeof(double), CL_MEM_READ_ONLY };
			CLArg arg2{ input.values, size * sizeof(double), CL_MEM_WRITE_ONLY };

			startCL(
				"CopyArray.cl", "copy_array",
				1, new size_t(size), new size_t(1),
				new CLArg[2]{ arg1, arg2 }, 2, true
			);
		}

		// Get Activation Values
		LayerValues* activations = new LayerValues[layer_num];
		activations[0] = connectLayer(input, layers, 1);
		for (unsigned int i = 1; i < layer_num; i++) {
			activations[i] = connectLayer(activations[i - 1], (layers + i), 1);
		}

		// Initialize Activation Updates
		LayerValues* activations_d = new LayerValues[layer_num];
		for (unsigned int i = 0; i < layer_num; i++) {
			size_t size = (size_t)activations[i].rows * (size_t)activations[i].cols;
			activations_d[i].rows = activations[i].rows;
			activations_d[i].cols = activations[i].cols;
			activations_d[i].values = new double[size];
			for (unsigned int v = 0; v < size; v++) {
				activations_d[i].values[v] = 0;
			}
		}

		// Calculate Error
		if (!label_is_derivative) {
			loss_mag = calculate_loss_mse(activations[layer_num - 1], training_label[tr_data]);
			std::cout << "Loss = " << loss_mag << "\n";
		}
		else {
			loss_mag = loss;
		}

		// Backpropagation
		for (unsigned int step = 0; step < layer_num; step++) {
			unsigned int layer = layer_num - step - 1;

			double learning_rate = loss_mag * layers[layer].learning_rate;

			// Calculate Loss Derivative in Proportion to Current Layer Activation
			if (layer == layer_num - 1) { // Output Layer
				size_t size = (size_t)activations_d[layer].rows * (size_t)activations_d[layer].cols;
				for (size_t index = 0; index < size; index++) {
					// Activation Output Derivative
					// Use Given Derivative for Chain Rule
					if (label_is_derivative)
						activations_d[layer].values[index] = training_label[tr_data].values[index];
					// Calculate Cost Derivative if Not Given Derivative
					else
						activations_d[layer].values[index] =
						2 * (activations[layer].values[index] - training_label[tr_data].values[index]);
					// Activation Function Derivative
					activations_d[layer].values[index] *= activation_derivative(activations[layer].values[index], layers[layer].activation_function);
				}
			}
			else { // Hidden Layer
				for (unsigned int row = 0; row < activations_d[layer].rows; row++) {
					for (unsigned int col = 0; col < activations_d[layer].cols; col++) {
						// Activation Output Derivative
						size_t index = (size_t)row * (size_t)activations_d[layer].cols + (size_t)col;
						double sum = 0;
						for (unsigned int i = 0; i < layers[layer + 1].getRows(); i++) {
							double value = layers[layer + 1].weights[i * layers[layer + 1].getCols() + row];
							value *= activations_d[layer + 1].values[i * activations_d[layer + 1].cols + col];
							sum += value;
						}
						activations_d[layer].values[index] = sum;
						// Activation Function Derivative
						activations_d[layer].values[index] *= activation_derivative(activations[layer].values[index], layers[layer].activation_function);
					}
				}
			}

			// Calculate Input Derivative upon Reaching First Layer
			if (layer == 0) {
				// Initialize Input Derivative for Current Iteration
				input_d[tr_data].rows = training_data[tr_data].rows;
				input_d[tr_data].cols = training_data[tr_data].cols;
				size_t size = (size_t)input_d[tr_data].rows * (size_t)input_d[tr_data].cols;
				input_d[tr_data].values = new double[size];

				// Calculate Input Derivative
				for (unsigned int row = 0; row < input_d[tr_data].rows; row++) {
					for (unsigned int col = 0; col < input_d[tr_data].cols; col++) {
						size_t index = (size_t)row * (size_t)input_d[tr_data].cols + (size_t)col;
						double sum = 0;

						for (unsigned int i = 0; i < layers[0].getRows(); i++) {
							double value = layers[0].weights[i * layers[0].getCols() + row];
							value *= activations_d[0].values[i * activations_d[0].cols + col];
							sum += value;
						}

						input_d[tr_data].values[index] = sum;
					}
				}

				// Save to Output
				input_derivative_output = input_d;
			}


			// Get Previous Layer Activation
			// Use Input Vector as Previous Layer Activation for the First Layer
			LayerValues prev_activations;
			if (layer > 0) prev_activations = activations[layer - 1];
			else prev_activations = input;

			// Get Weights Update
			// Calculate Loss Derivative in Proportion to Current Layer Weights
			for (unsigned int row = 0; row < layers[layer].getRows(); row++) {
				for (unsigned int col = 0; col < layers[layer].getCols(); col++) {
					unsigned int index = row * layers[layer].getCols() + col;
					double sum = 0;
					for (unsigned int i = 0; i < prev_activations.cols; i++) {
						double value = prev_activations.values[col * prev_activations.cols + i];
						value *= activations_d[layer].values[row * activations_d[layer].cols + i];
						sum += value;
					}
					weights_d[layer][index] -= sum * learning_rate;
				}
			}

			// Get Biases Update
			// Calculate Loss Derivative in Proportion to Current Layer Biases
			for (unsigned int index = 0; index < layers[layer].getRows(); index++) {
				double sum = 0;
				for (unsigned int i = 0; i < activations_d[layer].cols; i++) {
					sum += activations_d[layer].values[index * activations_d[layer].cols + i];
				}
				biases_d[layer][index] -= sum * learning_rate;
			}
		}
	}

	// Update Values
	if (update_layers) {
		// Update Actual Weights
		for (unsigned int i = 0; i < layer_num; i++) {
			CLArg arg1{ layers[i].weights, layers[i].num_weights * sizeof(double), CL_MEM_READ_WRITE };
			CLArg arg2{ weights_d[i], layers[i].num_weights * sizeof(double), CL_MEM_READ_ONLY };

			startCL(
				"AddArray.cl", "add_to_array",
				1, new size_t(layers[i].num_weights), new size_t(1),
				new CLArg[2]{ arg1, arg2 }, 2, true
			);
		}

		// Update Actual Biases
		for (unsigned int i = 0; i < layer_num; i++) {
			CLArg arg1{ layers[i].biases, layers[i].num_output * sizeof(double), CL_MEM_READ_WRITE };
			CLArg arg2{ biases_d[i], layers[i].num_output * sizeof(double), CL_MEM_READ_ONLY };
			startCL(
				"AddArray.cl", "add_to_array",
				1, new size_t(layers[i].num_weights), new size_t(1),
				new CLArg[2]{ arg1, arg2 }, 2, true
			);
		}
	}

	// Values Returned
	return loss_mag;
}