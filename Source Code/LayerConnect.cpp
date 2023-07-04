#include "Layer.hpp"

#include <cmath>
#include "MatrixMultiply.hpp"

namespace {
	void relu_array(double* values, size_t size) {
		try {
			startCL(
				"ReLU.cl", "relu",
				1, new size_t(size), new size_t(1),
				new CLArg[1]{ values, size * sizeof(double), CL_MEM_READ_WRITE }, 1, true
			);
		}
		catch (const char* e) {
			throw e;
		}
	}
	void tanh_array(double* values, size_t size) {
		for (size_t i = 0; i < size; i++) {
			values[i] = tanh(values[i] * 0.1);
		}
	}
	void activation_array(double* values, size_t size, unsigned int activation_function = 0) {
		if (activation_function == 0) {
			tanh_array(values, size);
			return;
		}
		if (true) {
			relu_array(values, size);
			return;
		}
	}
}

LayerValues connectLayer(const LayerValues& inputVector, const Layer* layers, unsigned int layer_num) {
	// Initialize result vector
	LayerValues result{ nullptr, 0, 0 };

	// Copy input vector to result vector
	if (true) {
		result.rows = inputVector.rows;
		result.cols = inputVector.cols;

		size_t size = (size_t)inputVector.rows * (size_t)inputVector.cols;
		result.values = new double[size];

		CLArg arg1{ inputVector.values, size * sizeof(double), CL_MEM_READ_ONLY };
		CLArg arg2{ result.values, size * sizeof(double), CL_MEM_WRITE_ONLY };

		startCL(
			"CopyArray.cl", "copy_array",
			1, new size_t(size), new size_t(1),
			new CLArg[2]{ arg1, arg2 }, 2, true
		);
	}

	// For every layer,
	for (unsigned int i = 0; i < layer_num; i++) {
		// Check for invalid size
		if (layers[i].num_input != result.rows) throw "Invalid layer input/output size";

		// Store multiplication result in temporary array
		size_t size = (size_t)layers[i].num_output * (size_t)result.cols;
		double* temp = new double[size];
		matrix_multiply(layers[i].weights, result.values, temp, layers[i].num_output, layers[i].num_input, result.cols);

		// Resize result vector
		delete[] result.values;
		result.rows = layers[i].num_output;
		result.values = new double[size];

		// Add biases and store to result vector
		if (true) {
			CLArg arg1{ temp, size * sizeof(double), CL_MEM_READ_ONLY};
			CLArg arg2{ layers[i].biases, layers[i].num_output * sizeof(double), CL_MEM_READ_ONLY};
			CLArg arg3{ result.values, size * sizeof(double), CL_MEM_WRITE_ONLY};
			startCL(
				"VectorAdd.cl", "vector_add",
				2, new size_t[2]{ result.rows, result.cols }, new size_t[2]{ 1, 1 },
				new CLArg[3]{ arg1, arg2, arg3 }, 3, true
			);
			delete[] temp;
		}

		// Activation Function for Layers
		activation_array(result.values, size, layers[i].activation_function);
	}

	return result;
}