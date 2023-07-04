#pragma once

#include <string>

class Layer;
struct LayerValues;
LayerValues convolute(const LayerValues& input, const LayerValues& layer);
LayerValues d_convolute(
	const LayerValues& filter,
	const LayerValues& cost_derivative,
	unsigned int input_row, unsigned int input_col
);
LayerValues connectLayer(
	const LayerValues& inputVector,
	const Layer* layers, unsigned int layer_num
);
double trainLayer(
	const Layer* layers, unsigned int layer_num,
	const LayerValues* training_data, const LayerValues* training_label, unsigned int training_num = 1,
	bool update_layers = true, bool label_is_derivative = false, const double& loss = 1
);
double trainLayer(
	LayerValues*& input_derivative_output,
	const Layer* layers, unsigned int layer_num,
	const LayerValues* training_data, const LayerValues* training_label, unsigned int training_num = 1,
	bool update_layers = true, bool label_is_derivative = false, const double& loss = 1
);

class Layer {
private:
	double* weights = nullptr;
	double* biases = nullptr;
	unsigned int num_input;             // columns
	unsigned int num_output;            // rows
	unsigned long long int num_weights; // length

	friend LayerValues connectLayer(
		const LayerValues& inputVector,
		const Layer* layers, unsigned int layer_num
	);
	friend double trainLayer(
		LayerValues*& input_derivative_output,
		const Layer* layers, unsigned int layer_num,
		const LayerValues* training_data, const LayerValues* training_label, unsigned int training_num,
		bool update_layers, bool label_is_derivative, double const& loss
	);

public:
	// Hyperparameters
	unsigned int activation_function = 0; // Activation Function
	double learning_rate = 0.1; // Learning Rate

	// Constructors
	Layer();
	Layer(unsigned int num_input_nodes, unsigned int num_output_nodes);
	Layer(const char* filename);
	~Layer();

	// Randomize Weights
	void initWeights(unsigned long long int index = 0);

	// File save/load
	void save(std::string filename) const;
	void load(std::string filename);

	// Operator == for debugging
	bool operator==(const Layer& operand) const;

	// Accessors
	double getValue(unsigned long long int index) const;
	LayerValues getValues() const;
	unsigned int getNumInput() const;
	unsigned int getNumOutput() const;
	unsigned int getRows() const;
	unsigned int getCols() const;
};

struct LayerValues {
	double* values = nullptr;  // When used as input:
	unsigned int rows = 0;     // Num of attributes per data
	unsigned int cols = 1;     // Num of data
};