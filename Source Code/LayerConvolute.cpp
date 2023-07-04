#include "Layer.hpp"

#include "CLWrapper.hpp"

#include <iostream>

LayerValues convolute(const LayerValues& input, const LayerValues& layer) {
	// Input Size Check
	if (input.rows < layer.rows) throw "Invalid layer size";
	if (input.cols < layer.cols) throw "Invalid layer size";

	// Initialize result vector
	size_t size;
	LayerValues result;
	result.rows = input.rows - layer.rows + 1;
	result.cols = input.cols - layer.cols + 1;
	size = (size_t)result.rows * (size_t)result.cols;
	result.values = new double[size];

	for (size_t x = 0; x < result.rows; x++) {
		for (size_t y = 0; y < result.cols; y++) {
			size_t result_index = x * result.cols + y;
			result.values[result_index] = 0;

			for (size_t i = 0; i < layer.rows; i++) {
				for (size_t v = 0; v < layer.cols; v++) {
					size_t layer_index = i * layer.cols + v;
					size_t input_index = (x + i) * input.cols + y + v;

					result.values[result_index] +=
						input.values[input_index] * layer.values[layer_index];
				}
			}
		}
	}

	// For unknown reason, using GPU to parallelize convolution randomly results in nan output
	/*
	CLArg* args = new CLArg[5];

	args[0].values = input.values;
	args[0].byte_size = (size_t)input.rows * (size_t)input.cols * sizeof(double);
	args[0].flags = CL_MEM_READ_ONLY;
	
	args[1].values = new unsigned int[2]{ input.rows, input.cols };
	args[1].byte_size = 2 * sizeof(unsigned int);
	args[1].flags = CL_MEM_READ_ONLY;

	args[2].values = layer.values;
	args[2].byte_size = (size_t)layer.rows * (size_t)layer.cols * sizeof(double);
	args[2].flags = CL_MEM_READ_ONLY;

	args[3].values = new unsigned int[2]{ layer.rows, layer.cols };
	args[3].byte_size = 2 * sizeof(unsigned int);
	args[3].flags = CL_MEM_READ_ONLY;

	args[4].values = result.values;
	args[4].byte_size = (size_t)result.rows * (size_t)result.cols * sizeof(double);
	args[4].flags = CL_MEM_WRITE_ONLY;
	
	try {
		startCL(
			"Convolute.cl", "convolute",
			2, new size_t[2]{ result.rows, result.cols }, new size_t[2]{ 1, 1 },
			args, 5, true
		);
	}
	catch (const char* e) {
		throw e;
	}
	/**/

	return result;
}

LayerValues d_convolute(
	const LayerValues& filter,
	const LayerValues& cost_derivative,
	unsigned int input_row, unsigned int input_col
) {
	// These values are initialized and known
	// LayerValues filter;
	// LayerValues cost_derivative;

	// This value is initialized with a known size, but with unknown values
	LayerValues derivative;
	derivative.rows = input_row;
	derivative.cols = input_col;
	derivative.values = new double[(size_t)input_row * (size_t)input_col];

	size_t filter_size = (size_t)filter.rows * (size_t)filter.cols;

	// Find value for every index in derivative matrix
	for (unsigned int row = 0; row < derivative.rows; row++) {
		for (unsigned int col = 0; col < derivative.cols; col++) {
			size_t index = (size_t)row * derivative.cols + (size_t)col;
			double sum = 0;

			// Iterate on every index in derivative matrix
			// For every index, take the same index from result matrix
			// Multiply it by filter, then increment filter position
			// When filter position is incremented, result matrix position is decremented
			// Continue to next iteration when filter position is exhausted
			// If result position is out of bound, then skip the multiplication and increment filter position

			for (unsigned int f_row = 0; f_row < filter.rows; f_row++) {
				for (unsigned int f_col = 0; f_col < filter.cols; f_col++) {
					// Check if index is out of bound
					if (row < f_row) continue;
					if (col < f_col) continue;

					size_t f_index = (size_t)f_row * filter.cols + (size_t)f_col;
					size_t v_index = (size_t)(row - f_row) * cost_derivative.cols + (size_t)(col - f_col);

					if (v_index >= (size_t)cost_derivative.rows * (size_t)cost_derivative.cols) continue;

					double value = cost_derivative.values[v_index];
					value *= filter.values[f_index];
					sum += value;
				}
			}

			derivative.values[index] = sum;
		}
	}

	return derivative;
}