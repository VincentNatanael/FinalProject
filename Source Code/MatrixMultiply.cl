__kernel void matrix_multiply(__global uint* size, __global double* mat1, __global double* mat2, __global double* result) {
	uint x = get_global_id(0);
	uint y = get_global_id(1);

	double sum = 0;
	for (int i = 0; i < size[1]; i++) {
		sum += mat1[x * size[1] + i] * mat2[i * size[2] + y];
	}

	result[x * size[2] + y] = sum;
}