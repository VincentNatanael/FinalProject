__kernel void vector_add(__global double* mat, __global double* vec, __global double* result) {
	uint x = get_global_id(0);
	uint y = get_global_id(1);
	uint x_max = get_global_size(0);
	uint y_max = get_global_size(1);

	double sum = mat[x * y_max + y] + vec[x];

	result[x * y_max + y] = sum;
}