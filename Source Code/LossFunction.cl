__kernel void loss(__global double* output, __global double* expected, __global double* result) {
	uint x = get_global_id(0);
	uint y = get_global_id(1);
	uint y_max = get_global_size(1);

	uint index = x * y_max + y;

	double loss = output[index] - expected[index];
	loss = loss * loss;

	result[index] = loss;
}

__kernel void loss_sum(__global double* loss, __global double* size, __global double* result) {
	uint i = get_global_id(0);

	double sum = 0;
	for (uint v = 0; v < size[0]; v++) {
		sum += loss[v * size[1] + i];
	}

	result[i] = sum;
}