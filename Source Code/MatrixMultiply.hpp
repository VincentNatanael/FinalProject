#pragma once

#include "CLWrapper.hpp"

bool matrix_multiply(
	double* mat1, double* mat2, double* result,
	unsigned int size_x, unsigned int size_n, unsigned int size_y
) {
	CLArg* args = new CLArg[4];

	args[0] = {
		new unsigned int[3]{ size_x, size_n, size_y },
		3 * sizeof(unsigned int),
		CL_MEM_READ_ONLY
	};

	args[1] = {
		mat1,
		(size_t)size_x * (size_t)size_n * sizeof(double),
		CL_MEM_READ_ONLY
	};

	args[2] = {
		mat2,
		(size_t)size_n * (size_t)size_y * sizeof(double),
		CL_MEM_READ_ONLY
	};

	args[3] = {
		result,
		(size_t)size_x * (size_t)size_y * sizeof(double),
		CL_MEM_WRITE_ONLY
	};

	size_t* global_size = new size_t[2]{ size_x, size_y };
	size_t* local_size = new size_t[2]{ 1, 1 };

	try {
		startCL(
			"MatrixMultiply.cl", "matrix_multiply",
			2, global_size, local_size,
			args, 4, true
		);
	}
	catch (const char* log) {
		throw log;
		return false;
	}

	return true;
}