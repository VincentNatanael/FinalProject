#include <iostream>
#include <sstream>
#include <fstream>
#include <cstdlib>

#include "Layer.hpp"
#include "MidiDecoder.hpp"
#include "CLWrapper.hpp"

// Initialize Constants
const LayerValues positive{ new double(1), 1, 1 };
const LayerValues negative{ new double(-1), 1, 1 };
const std::string dataset_list = "list.txt";
const unsigned int dataset_num = 53;
const unsigned int batch_training_num = 20;

// Hyperparameters
int k = -2;
unsigned int seed_size = 5000;
unsigned int g_layer_num = 20;
unsigned int g_layer_node_num = 100;
unsigned int d_layer_num = 10;
unsigned int d_layer_node_num = 50;
bool use_convolution = false;
double g_learning_rate = 0.1;
double d_learning_rate = 0.1;
double d_accuracy_target = 0.6;
std::string hyperparameters_list = "settings.txt";

// Initialize Layers
Layer* g_layer = new Layer[g_layer_num];
LayerValues c_layer{ new double[3] {-1, 0, 1}, 1, 3 };
Layer* d_layer = new Layer[d_layer_num];

std::string to_string(unsigned long long int integer) {
	std::stringstream stream;
	std::string str;
	stream << integer;
	stream >> str;
	return str;
}

void load_hyperparameters() {
	std::ifstream settings(hyperparameters_list);
	settings >> k;					settings.ignore(1000, '\n');
	settings >> seed_size;			settings.ignore(1000, '\n');
	settings >> g_layer_num;		settings.ignore(1000, '\n');
	settings >> g_layer_node_num;	settings.ignore(1000, '\n');
	settings >> d_layer_num;		settings.ignore(1000, '\n');
	settings >> d_layer_node_num;	settings.ignore(1000, '\n');
	settings >> use_convolution;	settings.ignore(1000, '\n');
	settings >> g_learning_rate;	settings.ignore(1000, '\n');
	settings >> d_learning_rate;	settings.ignore(1000, '\n');
	settings >> d_accuracy_target;	settings.ignore(1000, '\n');
	settings.close();
}

void create_new_layers() {
	delete[] g_layer;
	delete[] d_layer;
	g_layer = new Layer[g_layer_num];
	d_layer = new Layer[d_layer_num];

	Layer* temp = nullptr;

	// Generator First Layer
	temp = new Layer(seed_size, g_layer_node_num);
	temp->save("temp.bin");
	g_layer[0].load("temp.bin");
	delete temp;

	// Generator Hidden Layers
	for (int i = 1; i < g_layer_num-1; i++) {
		temp = new Layer(g_layer_node_num, g_layer_node_num);
		temp->save("temp.bin");
		g_layer[i].load("temp.bin");
		delete temp;
	}

	// Generator Last Layer
	temp = new Layer(g_layer_node_num, 802);
	temp->save("temp.bin");
	g_layer[g_layer_num-1].load("temp.bin");
	delete temp;

	// Discriminator First Layer
	if (use_convolution) {
		temp = new Layer(198 * 4 + 2, d_layer_node_num);
	}
	else {
		temp = new Layer(802, d_layer_node_num);
	}
	temp->save("temp.bin");
	d_layer[0].load("temp.bin");
	delete temp;

	// Discriminator Hidden Layers
	for (int i = 1; i < d_layer_num-1; i++) {
		temp = new Layer(d_layer_node_num, d_layer_node_num);
		temp->save("temp.bin");
		d_layer[i].load("temp.bin");
		delete temp;
	}

	// Discriminator Last Layer
	temp = new Layer(d_layer_node_num, 1);
	temp->save("temp.bin");
	d_layer[d_layer_num - 1].load("temp.bin");
	delete temp;
}

void load_layers(std::string prefix = "") {
	for (unsigned int i = 0; i < g_layer_num; i++) {
		g_layer[i].load(prefix + "g_layer_" + to_string(i) + ".bin");
	}
	for (unsigned int i = 0; i < d_layer_num; i++) {
		d_layer[i].load(prefix + "d_layer_" + to_string(i) + ".bin");
	}
}

void save_layers(std::string prefix = "") {
	for (unsigned int i = 0; i < g_layer_num; i++) {
		g_layer[i].save(prefix + "g_layer_" + to_string(i) + ".bin");
	}
	for (unsigned int i = 0; i < d_layer_num; i++) {
		d_layer[i].save(prefix + "d_layer_" + to_string(i) + ".bin");
	}
}

void normalize_dataset() {
	normalize(dataset_list);
}

LayerValues prepare_data(std::string filename) {
	// Load Training Data
	NormalizedTrainingData data;
	loadNormalizedData(data, filename);
	
	// Initialize Result Vector
	LayerValues result;
	result.rows = 802;
	result.cols = 1;
	result.values = new double[802];

	// Clean Bar Starts
	size_t bar_num = data.bar_starts.getSize();
	for (size_t i = 0; i < bar_num; i++) {
		if (data.bar_starts.peek() <= (data.notes.getSize() - 200))
			data.bar_starts.enqueue(data.bar_starts.dequeue());
		else
			data.bar_starts.dequeue();
	}

	// Choose Starting Index at Random
	unsigned int index;
	index = rand() % data.bar_starts.getSize();

	for (size_t i = 0; i < index; i++) {
		data.bar_starts.dequeue();
	}
	for (size_t i = 0; i < data.bar_starts.peek(); i++) {
		data.notes.dequeue();
	}

	index = 0;
	result.values[index++] = data.key;
	result.values[index++] = data.tempo;
	while (index < 802) {
		NormalizedMidiNote note = data.notes.dequeue();
		result.values[index++] = note.note;
		result.values[index++] = note.velocity;
		result.values[index++] = note.duration;
		result.values[index++] = note.delay;
	}

	return result;
}

LayerValues fetch_dataset() {
	// Opening Dataset List
	std::ifstream file(dataset_list);
	if (!file.is_open()) throw "Failed to open file";
	
	// Choose Index at Random
	unsigned int data_index = (unsigned int)rand() % dataset_num;

	// Get Data Filename
	std::string data_file = "";
	for (unsigned int i = 0; i < data_index; i++) std::getline(file, data_file);
	std::getline(file, data_file);

	// Process Data
	LayerValues data = prepare_data(data_file);

	// Return Processed Data
	return data;
}

void generate(std::string filename = "output") {
	// Declarations
	long long int values[802];
	char w_head = 0;
	std::string memory = "";
	Queue<size_t> active_notes(200);

	// Create Seed for Generator Input
	LayerValues g_input = Layer(1, seed_size).getValues();
	std::cout << "Seed Created\n";

	// Pass Seed to Generator
	LayerValues g_output = connectLayer(g_input, g_layer, g_layer_num);
	std::cout << "Generator Passed\n";

	std::cout << "Sample Generator Output: ";
	std::cout << g_output.values[0] << ", ";
	std::cout << g_output.values[1] << ", ";
	std::cout << g_output.values[2] << ", ";
	std::cout << g_output.values[3] << ", ";
	std::cout << g_output.values[4] << ", ";
	std::cout << g_output.values[5] << ", ";
	std::cout << g_output.values[6] << ", ";
	std::cout << g_output.values[7] << ", ";
	std::cout << g_output.values[8] << ", ";
	std::cout << g_output.values[9] << "\n";

	for (unsigned int i = 1; i < 802; i++) {
		if (g_output.values[i] < -1 || g_output.values[i] > 1) std::cout << "Denormalization Check Found Invalid Generator Output! (" << g_output.values[i] << " on index " << i << ")\n";
	}

	// Loading Normalization Info
	TrainingData normalization_info;
	loadTrainingData(normalization_info, "normalization_info");
	std::cout << "Normalization Info Loaded\n";

	// Denormalizing Generator Output
	for (unsigned int i = 0; i < 802; i++) {
		// Denormalization
		g_output.values[i] += 1;
		g_output.values[i] /= 2;
		// Key
		if (i == 0) {
			g_output.values[i] *= (normalization_info.key - normalization_info.key_min);
			g_output.values[i] += normalization_info.key_min;
		}
		// Tempo
		else if (i == 1) {
			g_output.values[i] *= (normalization_info.tempo - normalization_info.tempo_min);
			g_output.values[i] += normalization_info.tempo_min;
		}
		// Note
		else if ((i - 2) % 4 == 0) {
			g_output.values[i] *= (normalization_info.note_max - normalization_info.note_min);
			g_output.values[i] += normalization_info.note_min;
		}
		// Velocity
		else if ((i - 2) % 4 == 1) {
			g_output.values[i] *= (normalization_info.velocity_max - normalization_info.velocity_min);
			g_output.values[i] += normalization_info.velocity_min;
		}
		// Duration
		else if ((i - 2) % 4 == 2) {
			g_output.values[i] *= (normalization_info.duration_max - normalization_info.duration_min);
			g_output.values[i] += normalization_info.duration_min;
		}
		// Delay
		else if ((i - 2) % 4 == 3) {
			g_output.values[i] *= (normalization_info.delay_max - normalization_info.delay_min);
			g_output.values[i] += normalization_info.delay_min;
		}
		
		// Rounding
		long long int floor = g_output.values[i];
		double remainder = g_output.values[i] - floor;
		long long int rounded = floor;
		if (remainder >= 0.5) rounded++;
		if (remainder <= -0.5) rounded--;
		values[i] = rounded;
	}
	std::cout << "Generator Output Denormalized\n";

	std::cout << "Sample Values: ";
	std::cout << values[0] << ", ";
	std::cout << values[1] << ", ";
	std::cout << values[2] << ", ";
	std::cout << values[3] << ", ";
	std::cout << values[4] << ", ";
	std::cout << values[5] << ", ";
	std::cout << values[6] << ", ";
	std::cout << values[7] << ", ";
	std::cout << values[8] << ", ";
	std::cout << values[9] << "\n";

	// Checking Denormalization Result
	for (unsigned int i = 1; i < 802; i++) {
		if (values[i] < 0) std::cout << "Denormalization Check Found Negative Values! (" << values[i] << " on index " << i << ")\n";
	}

	// Preparing to Generate MIDI File
	std::ofstream midi(filename += ".mid", std::ios::binary);
	if (!midi.is_open()) throw "Unable to Create MIDI File";
	std::cout << "MIDI File Created\n";
	
	// Header
	if (true) {
		// Header Label
		w_head = 'M'; midi.write(&w_head, 1);
		w_head = 'T'; midi.write(&w_head, 1);
		w_head = 'h'; midi.write(&w_head, 1);
		w_head = 'd'; midi.write(&w_head, 1);
		// Header Length
		w_head = 0x00; midi.write(&w_head, 1);
		w_head = 0x00; midi.write(&w_head, 1);
		w_head = 0x00; midi.write(&w_head, 1);
		w_head = 0x06; midi.write(&w_head, 1);
		// MIDI Format
		w_head = 0x00; midi.write(&w_head, 1);
		w_head = 0x00; midi.write(&w_head, 1);
		// Number of Tracks
		w_head = 0x00; midi.write(&w_head, 1);
		w_head = 0x01; midi.write(&w_head, 1);
		// Time Division
		w_head = (480U & 0x00007F00) >> 8; midi.write(&w_head, 1);
		w_head = (480U & 0x000000FF) >> 0; midi.write(&w_head, 1);
	}
	std::cout << "Header Written to File\n";

	// Track Label
	if (true) {
		w_head = 'M'; midi.write(&w_head, 1);
		w_head = 'T'; midi.write(&w_head, 1);
		w_head = 'r'; midi.write(&w_head, 1);
		w_head = 'k'; midi.write(&w_head, 1);
	}
	std::cout << "Track Label Written to File\n";

	// Track Name
	if (true) {
		memory += (char)0x00;
		memory += (char)0xFF;
		memory += (char)0x03;
		memory += (char)0x06;
		memory += 'P';
		memory += 'i';
		memory += 'a';
		memory += 'n';
		memory += 'o';
		memory += (char)0x00;
	}
	std::cout << "Track Name to Memory\n";

	// Time Signature
	if (true) {
		memory += (char)0x00;
		memory += (char)0xFF;
		memory += (char)0x58;
		memory += (char)0x04;
		memory += (char)0x04;
		memory += (char)0x02;
		memory += (char)0x18;
		memory += (char)0x08;
	}
	std::cout << "Time Signature Written to Memory\n";

	// Key Signature
	if (true) {
		memory += (char)0x00;
		memory += (char)0xFF;
		memory += (char)0x59;
		memory += (char)0x02;

		char key = values[0];
		switch (key) {
		case  0: key =  0; break; // C
		case  1: key = -5; break; // Db
		case  2: key =  2; break; // D
		case  3: key = -3; break; // Eb
		case  4: key =  4; break; // E
		case  5: key = -1; break; // F
		case -6: key = -6; break; // Gb
		case -5: key =  1; break; // G
		case -4: key = -4; break; // Ab
		case -3: key =  3; break; // A
		case -2: key = -2; break; // Bb
		case -1: key =  5; break; // B
		}
		memory += key;

		memory += (char)0x00;
	}
	std::cout << "Key Signature Written to Memory\n";

	// Set Tempo
	if (true) {
		memory += (char)0x00;
		memory += (char)0xFF;
		memory += (char)0x51;
		memory += (char)0x03;
		unsigned int tempo = values[1] / 4;
		char byte3 = tempo & 0x000000FF; tempo >>= 8;
		char byte2 = tempo & 0x000000FF; tempo >>= 8;
		char byte1 = tempo & 0x000000FF; tempo >>= 8;
		memory += byte1;
		memory += byte2;
		memory += byte3;
	}
	std::cout << "Tempo Written to Memory\n";

	// Reset All Controller
	if (true) {
		memory += (char)0x00;
		memory += (char)0xB0;
		memory += (char)0x79;
		memory += (char)0x00;
	}

	// Program Change (Piano)
	if (true) {
		memory += (char)0x00;
		memory += (char)0xC0;
		memory += (char)0x00;
	}

	// Set Volume, Pan, Reverb, Chorus to Default
	if (true) {
		// Volume
		memory += (char)0x00;
		memory += (char)0xB0;
		memory += (char)0x07;
		memory += (char)0x64;
		// Pan
		memory += (char)0x00;
		memory += (char)0x0A;
		memory += (char)0x40;
		// Reverb Send
		memory += (char)0x00;
		memory += (char)0x5B;
		memory += (char)0x00;
		// Chorus Send
		memory += (char)0x00;
		memory += (char)0x5D;
		memory += (char)0x00;
	}

	// Port Prefix
	if (true) {
		memory += (char)0x00;
		memory += (char)0xFF;
		memory += (char)0x21;
		memory += (char)0x01;
		memory += (char)0x00;
	}
	std::cout << "Controller and Program Changes Written to Memory\n";

	// Write First Note to Memory
	if (true) {
		// Get Delay 7-bit Byte Length
		unsigned int length_byte = 1;
		if (false);
		else if ((values[5] & 0xF0000000) > 0) length_byte = 5;
		else if ((values[5] & 0x0FE00000) > 0) length_byte = 4;
		else if ((values[5] & 0x001FC000) > 0) length_byte = 3;
		else if ((values[5] & 0x00003F80) > 0) length_byte = 2;
		else if ((values[5] & 0x0000007F) > 0) length_byte = 1;
		// Write Delay in Variable Length
		switch (length_byte) {
		case 5:
			w_head = (values[5] & 0xF0000000) >> 28;
			w_head |= 0x80;
			memory += w_head;
		case 4:
			w_head = (values[5] & 0x0FE00000) >> 21;
			w_head |= 0x80;
			memory += w_head;
		case 3:
			w_head = (values[5] & 0x001FC000) >> 14;
			w_head |= 0x80;
			memory += w_head;
		case 2:
			w_head = (values[5] & 0x00003F80) >> 7;
			w_head |= 0x80;
			memory += w_head;
		case 1:
			w_head = (values[5] & 0x0000007F) >> 0;
			w_head &= 0x7F;
			memory += w_head;
			break;
		}
		// Write Note On Event
		memory += (char)0x90;
		memory += (char)(values[2]+values[0]);
		memory += (char)values[3];
		// Enqueue Note Off Event
		active_notes.enqueue((size_t)(values[2] << 32) + (size_t)values[4]);
	}

	// Write Following Notes to Memory
	for (unsigned int i = 6; i < 802; i += 4) {
		// Get Minimum Delay
		unsigned int min_delay = values[i + 3];
		size_t active_note_num = active_notes.getSize();
		for (unsigned int v = 0; v < active_note_num; v++) {
			unsigned int end_delay = active_notes.peek();
			if (min_delay > end_delay) min_delay = end_delay;
			active_notes.enqueue(active_notes.dequeue());
		}

		// Subtract Note Duration with Minimum Delay, Write Note Off to Memory on Zero
		for (unsigned int v = 0; v < active_note_num; v++) {
			size_t note_end = active_notes.dequeue();
			note_end -= min_delay;
			if ((note_end & 0x00000000FFFFFFFF) == 0) {
				note_end >>= 32;
				// Get Delay 7-bit Byte Length
				unsigned int length_byte = 1;
				if (false);
				else if ((min_delay & 0xF0000000) > 0) length_byte = 5;
				else if ((min_delay & 0x0FE00000) > 0) length_byte = 4;
				else if ((min_delay & 0x001FC000) > 0) length_byte = 3;
				else if ((min_delay & 0x00003F80) > 0) length_byte = 2;
				else if ((min_delay & 0x0000007F) > 0) length_byte = 1;
				// Write Delay in Variable Length
				switch (length_byte) {
				case 5:
					w_head  = (min_delay & 0xF0000000) >> 28;
					w_head |= 0x80;
					memory += w_head;
				case 4:
					w_head  = (min_delay & 0x0FE00000) >> 21;
					w_head |= 0x80;
					memory += w_head;
				case 3:
					w_head  = (min_delay & 0x001FC000) >> 14;
					w_head |= 0x80;
					memory += w_head;
				case 2:
					w_head  = (min_delay & 0x00003F80) >> 7;
					w_head |= 0x80;
					memory += w_head;
				case 1:
					w_head  = (min_delay & 0x0000007F) >> 0;
					w_head &= 0x7F;
					memory += w_head;
					break;
				}
				// Write Note Key and Zero Velocity
				memory += (char)note_end;
				memory += (char)0x00;
			}
			else active_notes.enqueue(note_end);
		}

		// Subtrack Note Delay with Minimum Delay, Write to Memory on Zero
		values[i + 3] -= min_delay;
		if (values[i + 3] == 0) {
			// Get Delay 7-bit Byte Length
			unsigned int length_byte = 1;
			if (false);
			else if ((min_delay & 0xF0000000) > 0) length_byte = 5;
			else if ((min_delay & 0x0FE00000) > 0) length_byte = 4;
			else if ((min_delay & 0x001FC000) > 0) length_byte = 3;
			else if ((min_delay & 0x00003F80) > 0) length_byte = 2;
			else if ((min_delay & 0x0000007F) > 0) length_byte = 1;
			// Write Delay in Variable Length
			switch (length_byte) {
			case 5:
				w_head = (min_delay & 0xF0000000) >> 28;
				w_head |= 0x80;
				memory += w_head;
			case 4:
				w_head = (min_delay & 0x0FE00000) >> 21;
				w_head |= 0x80;
				memory += w_head;
			case 3:
				w_head = (min_delay & 0x001FC000) >> 14;
				w_head |= 0x80;
				memory += w_head;
			case 2:
				w_head = (min_delay & 0x00003F80) >> 7;
				w_head |= 0x80;
				memory += w_head;
			case 1:
				w_head = (min_delay & 0x0000007F) >> 0;
				w_head &= 0x7F;
				memory += w_head;
				break;
			}
			// Write Note On Event
			memory += (char)(values[i]+values[0]);
			memory += (char)values[i + 1];
			// Enqueue Note Off Event
			active_notes.enqueue((size_t)((values[i]+values[0]) << 32) + (size_t)values[i + 2]);
		}
		else i -= 4;
	}
	std::cout << "Note Events Written to Memory\n"; 0x05EF;

	// End of Track
	if (true) {
		memory += (char)0x01;
		memory += (char)0xFF;
		memory += (char)0x2F;
		memory += (char)0x00;
	}
	std::cout << "End of Track Event Written to Memory\n";

	// Get Track Size
	size_t length = memory.size(); 0x0393;

	// Write Track Size
	if (true) {
		w_head = (length & 0xFF000000) >> 24; midi.write(&w_head, 1);
		w_head = (length & 0x00FF0000) >> 16; midi.write(&w_head, 1);
		w_head = (length & 0x0000FF00) >>  8; midi.write(&w_head, 1);
		w_head = (length & 0x000000FF) >>  0; midi.write(&w_head, 1);
	}
	std::cout << "Track Size Written to File\n";

	// Write Memory to File
	for (size_t i = 0; i < length; i++) {
		w_head = memory[i]; midi.write(&w_head, 1);
	}
	std::cout << "Memory Written to File\n";

	midi.close();
	std::cout << "MIDI Generated\n";
}

double forward_pass_generator() {
	// Create Seed for Generator Input
	LayerValues g_input = Layer(1, seed_size).getValues();

	// Pass Seed to Generator
	LayerValues g_output = connectLayer(g_input, g_layer, g_layer_num);

	// Prepare Discriminator Input
	LayerValues d_input;
	if (use_convolution) {
		// Convolute Generator Output
		LayerValues c_input;
		c_input.rows = 4;
		c_input.cols = 200;
		c_input.values = new double[800];
		for (int i = 0; i < 800; i++) {
			c_input.values[i] = g_output.values[i + 2];
		}
		LayerValues c_output = convolute(c_input, c_layer);

		// Use Convolution Output as Discriminator Input
		d_input.rows = 198 * 4 + 2;
		d_input.cols = 1;
		d_input.values = new double[d_input.rows];
		d_input.values[0] = g_output.values[0];
		d_input.values[1] = g_output.values[1];
		for (unsigned int i = 2; i < d_input.rows; i++) {
			d_input.values[i] = c_output.values[i - 2];
		}
	}
	else {
		// Use Generator Output as Discriminator Input
		d_input.rows = g_output.rows;
		d_input.cols = g_output.cols;
		d_input.values = g_output.values;
	}

	// Pass to Discriminator
	LayerValues d_output = connectLayer(d_input, d_layer, d_layer_num);

	return d_output.values[0];
}

double forward_pass_dataset(LayerValues data) {
	// Prepare Discriminator Input
	LayerValues d_input;
	if (use_convolution) {
		// Convolute Data form Dataset
		LayerValues c_input;
		c_input.rows = 4;
		c_input.cols = 200;
		c_input.values = new double[800];
		for (int i = 0; i < 800; i++) {
			c_input.values[i] = data.values[i + 2];
		}
		LayerValues c_output = convolute(c_input, c_layer);

		// Use Convolution Output as Discriminator Input
		d_input.rows = 198 * 4 + 2;
		d_input.cols = 1;
		d_input.values = new double[d_input.rows];
		d_input.values[0] = data.values[0];
		d_input.values[1] = data.values[1];
		for (unsigned int i = 2; i < d_input.rows; i++) {
			d_input.values[i] = c_output.values[i - 2];
		}
	}
	else {
		// Use Data from Dataset as Discriminator Input
		d_input.rows = data.rows;
		d_input.cols = data.cols;
		d_input.values = data.values;
	}

	// Pass to Discriminator
	LayerValues d_output = connectLayer(d_input, d_layer, d_layer_num);

	return d_output.values[0];
}

double train_discriminator_negative() {
	// Create Seed for Generator Input
	LayerValues g_input = Layer(1, seed_size).getValues();

	// Pass Seed to Generator
	LayerValues g_output = connectLayer(g_input, g_layer, g_layer_num);

	// Prepare Discriminator Input
	LayerValues d_input;
	if (use_convolution) {
		// Convolute Generator Output
		LayerValues c_input;
		c_input.rows = 4;
		c_input.cols = 200;
		c_input.values = new double[800];
		for (int i = 0; i < 800; i++) {
			c_input.values[i] = g_output.values[i + 2];
		}
		LayerValues c_output = convolute(c_input, c_layer);

		// Use Convolution Output as Discriminator Input
		d_input.rows = 198 * 4 + 2;
		d_input.cols = 1;
		d_input.values = new double[d_input.rows];
		d_input.values[0] = g_output.values[0];
		d_input.values[1] = g_output.values[1];
		for (unsigned int i = 2; i < d_input.rows; i++) {
			d_input.values[i] = c_output.values[i - 2];
		}
	}
	else {
		// Use Generator Output as Discriminator Input
		d_input.rows = g_output.rows;
		d_input.cols = g_output.cols;
		d_input.values = g_output.values;
	}

	// Train Discriminator with Generator Output
	double loss = trainLayer(d_layer, d_layer_num, &d_input, &negative, 1, true, false, loss);
	return loss;
}

double train_discriminator_positive(LayerValues data) {
	// Prepare Discriminator Input
	LayerValues d_input;
	if (use_convolution) {
		// Convolute Data form Dataset
		LayerValues c_input;
		c_input.rows = 4;
		c_input.cols = 200;
		c_input.values = new double[800];
		for (int i = 0; i < 800; i++) {
			c_input.values[i] = data.values[i + 2];
		}
		LayerValues c_output = convolute(c_input, c_layer);

		// Use Convolution Output as Discriminator Input
		d_input.rows = 198 * 4 + 2;
		d_input.cols = 1;
		d_input.values = new double[d_input.rows];
		d_input.values[0] = data.values[0];
		d_input.values[1] = data.values[1];
		for (unsigned int i = 2; i < d_input.rows; i++) {
			d_input.values[i] = c_output.values[i - 2];
		}
	}
	else {
		// Use Data from Dataset as Discriminator Input
		d_input.rows = data.rows;
		d_input.cols = data.cols;
		d_input.values = data.values;
	}

	// Train Discriminator with Input
	double loss = trainLayer(d_layer, d_layer_num, &d_input, &positive, 1, true, false, loss);
	return loss;
}

double train_generator() {
	// Create Seed for Generator Input
	LayerValues g_input = Layer(1, seed_size).getValues();

	// Pass Seed to Generator
	LayerValues g_output = connectLayer(g_input, g_layer, g_layer_num);

	// Prepare Discriminator Input
	LayerValues d_input;
	if (use_convolution) {
		// Convolute Generator Output
		LayerValues c_input;
		c_input.rows = 4;
		c_input.cols = 200;
		c_input.values = new double[800];
		for (int i = 0; i < 800; i++) {
			c_input.values[i] = g_output.values[i + 2];
		}
		LayerValues c_output = convolute(c_input, c_layer);

		// Use Convolution Output as Discriminator Input
		d_input.rows = 198 * 4 + 2;
		d_input.cols = 1;
		d_input.values = new double[d_input.rows];
		d_input.values[0] = g_output.values[0];
		d_input.values[1] = g_output.values[1];
		for (unsigned int i = 2; i < d_input.rows; i++) {
			d_input.values[i] = c_output.values[i - 2];
		}
	}
	else {
		// Use Generator Output as Discriminator Input
		d_input.rows = g_output.rows;
		d_input.cols = g_output.cols;
		d_input.values = g_output.values;
	}

	// Train Discriminator with Opposite Values
	// Do Not Update Discriminator
	LayerValues* d_input_derivative;
	double loss = trainLayer(d_input_derivative, d_layer, d_layer_num, &d_input, &positive, 1, false);

	// Train Generator
	// Requires Cost Calculation to be Skipped and Use Cost Derivative from Passed Parameter Instead
	trainLayer(g_layer, g_layer_num, &g_input, d_input_derivative, 1, true, true, loss);

	return loss;
}

double train_batch() {
	// Declarations
	double loss = 1;
	unsigned int correct = 0;
	unsigned int incorrect = 0;

	// Num of Training in Every Iteration
	for (unsigned int itr = 0; itr < batch_training_num; itr++) {
		// Train Discriminator K Times if K is positive
		// Otherwise, Train Discriminator 1 Time
		if (k > 0) {
			for (int i = 0; i < k / 2; i++) {
				std::cout << "Training Discriminator . . .\n";
				// Train with data from generator
				loss = train_discriminator_negative();
				if (loss < 1) correct++;
				else incorrect++;

				// Train with data from dataset
				LayerValues data = fetch_dataset();
				loss = train_discriminator_positive(data);
				if (loss < 1) correct++;
				else incorrect++;
			}
			if (k % 2 == 1) {
				std::cout << "Training Discriminator . . .\n";
				// Train with random data for odd k
				if (rand() % 2 == 0) {
					loss = train_discriminator_negative();
					if (loss < 1) correct++;
					else incorrect++;
					std::cout << "Loss = -\n";
				}
				else {
					std::cout << "Loss = -\n";
					LayerValues data = fetch_dataset();
					loss = train_discriminator_positive(data);
					if (loss < 1) correct++;
					else incorrect++;
				}
			}
		}
		else {
			std::cout << "Training Discriminator . . .\n";
			// Train with random data for odd k
			if (rand() % 2 == 0) {
				loss = train_discriminator_negative();
				if (loss < 1) correct++;
				else incorrect++;
				std::cout << "Loss = -\n";
			}
			else {
				std::cout << "Loss = -\n";
				LayerValues data = fetch_dataset();
				loss = train_discriminator_positive(data);
				if (loss < 1) correct++;
				else incorrect++;
			}
		}

		// Train Generator K Times if K is Negative
		// Otherwise, Train Generator 1 Time
		if (k < 0) {
			for (int i = 0; i < -k; i++) {
				std::cout << "Training Generator . . .\n";
				train_generator();
			}
		}
		else {
			std::cout << "Training Generator . . .\n";
			train_generator();
		}
	}

	// Accuracy Check for Early Training Optimization
	return (double)correct / (double)(correct+incorrect);
}

double test_accuracy(unsigned int test_num = 1) {
	unsigned int test_correct = 0;
	for (unsigned int i = 0; i < test_num; i++) {
		double d_result = forward_pass_generator();
		if (d_result < 0) test_correct++;
	}
	for (unsigned int i = 0; i < test_num; i++) {
		LayerValues data = fetch_dataset();
		double d_result = forward_pass_dataset(data);
		if (d_result > 0) test_correct++;
	}
	double accuracy = (double)test_correct / ((double)test_num * 2);
	return accuracy;
}

void train_network(size_t batch = 0) {
	// Declarations
	const unsigned int test_num = 20;
	const double accuracy_threshold = 0.8;
	const double accuracy_break = d_accuracy_target;
	double accuracy = 0;
	bool accuracy_d = false;

	if (batch != 0) {
		std::string prefix = "backup_";
		prefix += to_string(batch);
		prefix += "_";
		load_layers(prefix);
	}
	batch++;

	// Train Network, Break When Accuracy < 0.6
	do {
		// Train Network
		std::cout << "Training Batch " << batch++ << "\n";
		double acc = train_batch();

		std::cout << "Training Accuracy: " << acc << "\n";

		// Saving Backup
		std::string backup_prefix = "backup_";
		backup_prefix += to_string(batch - 1);
		backup_prefix += "_";
		save_layers(backup_prefix);

		// Test Accuracy
		if (acc == 1) accuracy_d = true;
		if (acc < 1) {
			std::cout << "Testing Accuracy . . .\n";
			accuracy = test_accuracy(test_num);
			std::cout << "Accuracy: " << accuracy << "\n";
			if (accuracy > accuracy_threshold) accuracy_d = true;
		}
		else accuracy = 1;
	} while (accuracy > accuracy_break || !accuracy_d);

	system("pause");
}

int main(int argc, char* argv[]) {
	load_hyperparameters();

	srand(time(NULL));
	for (unsigned int i = 0; i < g_layer_num; i++) {
		g_layer[i].learning_rate = g_learning_rate;
	}
	for (unsigned int i = 0; i < d_layer_num; i++) {
		d_layer[i].learning_rate = d_learning_rate;
	}

	while (true) {
		system("cls");
		unsigned int menu = -1;
		std::string filename = "";

		std::cout << "Menu:\n";
		std::cout << "1. Generate\n";
		std::cout << "2. Train\n";
		std::cout << "3. Load\n";
		std::cout << "4. Save\n";
		std::cout << "5. Test\n";
		std::cout << "6. Initialize\n";
		std::cout << "7. Normalize\n";
		std::cout << "0. Exit\n";

		std::cout << "\nInput: ";
		std::cin >> menu;

		switch (menu) {
		case 0: // Exit
			return 0;
			break;
		case 1: // Generate MIDI
			std::cout << "Filename: ";
			std::getchar();
			std::getline(std::cin, filename);
			//std::cout << "Generating . . . ";
			generate(filename);
			//std::cout << "Complete!\n";
			system("pause");
			break;
		case 2: // Train Layers
			std::cout << "Iteration: ";
			std::cin >> menu;
			train_network(menu);
			system("pause");
			break;
		case 3: // Load Layers
			std::cout << "Iteration: ";
			std::cin >> menu;
			if (menu > 0) load_layers("backup_" + to_string(menu) + "_");
			else load_layers();
			break;
		case 4: // Save Layers
			save_layers();
			break;
		case 5: // Test
			{
				std::cout << "Batch: ";
				std::cin >> menu;
				std::ofstream log("log.txt");
				for (unsigned int i = 0; i <= menu; i++) {
					if (i > 0) load_layers("backup_" + to_string(i) + "_");
					else load_layers();
					std::cout << "Testing accuracy for batch " << i << " . . .\n";
					double accuracy = test_accuracy(20);
					std::cout << "Accuracy: " << accuracy << "\n";
					log << i << " " << accuracy << "\n";
				}
				log.close();
				system("pause");
			}
			break;
		case 6: // Create New Layers
			create_new_layers();
			break;
		case 7: // Normalize Dataset
			normalize_dataset();
			try {
			}
			catch (const char* e) {
				std::cout << e << "\n";
			}
			system("pause");
			break;
		case 8: // Debug Convolution
		{
			Layer debugLayer(5, 5);
			LayerValues debugInput = debugLayer.getValues();
			LayerValues debugOutput = convolute(debugInput, c_layer);
			LayerValues debugOutput2 = d_convolute(c_layer, debugOutput, 5, 5);
			// Print Input
			std::cout << "Input:\n";
			for (unsigned int i = 0; i < debugInput.rows; i++) {
				for (unsigned int v = 0; v < debugInput.cols; v++) {
					std::cout << debugInput.values[i * debugInput.cols + v] << " ";
				}
				std::cout << "\n";
			}
			std::cout << "\n";
			// Print Convolution Output
			std::cout << "Convolution Output:\n";
			for (unsigned int i = 0; i < debugOutput.rows; i++) {
				for (unsigned int v = 0; v < debugOutput.cols; v++) {
					std::cout << debugOutput.values[i * debugOutput.cols + v] << " ";
				}
				std::cout << "\n";
			}
			std::cout << "\n";
			// Print Deconvolution Output
			std::cout << "Deconvolution Output:\n";
			for (unsigned int i = 0; i < debugOutput2.rows; i++) {
				for (unsigned int v = 0; v < debugOutput2.cols; v++) {
					std::cout << debugOutput2.values[i * debugOutput2.cols + v] << " ";
				}
				std::cout << "\n";
			}
			std::cout << "\n";
			system("pause");
		}
		break;
		default: // Invalid Input
			break;
		}
	}
}