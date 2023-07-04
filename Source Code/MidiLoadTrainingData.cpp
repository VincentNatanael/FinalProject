#include "MidiDecoder.hpp"

#include <fstream>

void loadTrainingData(TrainingData& output, std::string filename) {
	// Integrity Check
	std::ifstream file(filename + ".bin", std::ios::binary);
	if (!file.is_open()) throw "Failed to open file";
	while (!output.notes.isEmpty()) output.notes.dequeue();
	while (!output.bar_starts.isEmpty()) output.bar_starts.dequeue();

	// Tempo and Key
	file.read((char*)&output.tempo_min, sizeof(unsigned int));
	file.read((char*)&output.tempo, sizeof(unsigned int));
	file.read((char*)&output.key_min, sizeof(char));
	file.read((char*)&output.key, sizeof(char));

	// Normalization Info
	file.read((char*)&output.note_min, sizeof(unsigned int));
	file.read((char*)&output.note_max, sizeof(unsigned int));
	file.read((char*)&output.velocity_min, sizeof(unsigned int));
	file.read((char*)&output.velocity_max, sizeof(unsigned int));
	file.read((char*)&output.duration_min, sizeof(unsigned int));
	file.read((char*)&output.duration_max, sizeof(unsigned int));
	file.read((char*)&output.delay_min, sizeof(unsigned int));
	file.read((char*)&output.delay_max, sizeof(unsigned int));

	// Enqueue All Notes
	unsigned long long int size;
	file.read((char*)&size, sizeof(unsigned long long int));
	output.notes.resize(size);
	for (unsigned long long int i = 0; i < size; i++) {
		MidiNote temp;
		file.read((char*)&temp.note, sizeof(unsigned int));
		file.read((char*)&temp.velocity, sizeof(unsigned int));
		file.read((char*)&temp.duration, sizeof(unsigned int));
		file.read((char*)&temp.delay, sizeof(unsigned int));
		output.notes.enqueue(temp);
	}

	file.read((char*)&size, sizeof(unsigned long long int));
	output.bar_starts.resize(size);
	for (unsigned long long int i = 0; i < size; i++) {
		unsigned long long int temp;
		file.read((char*)&temp, sizeof(unsigned long long int));
		output.bar_starts.enqueue(temp);
	}

	file.close();
}

void loadNormalizedData(NormalizedTrainingData& output, std::string filename) {
	// Integrity Check
	std::ifstream file(filename + "_normalized.bin", std::ios::binary);
	if (!file.is_open()) throw "Failed to open file";
	while (!output.notes.isEmpty()) output.notes.dequeue();
	while (!output.bar_starts.isEmpty()) output.bar_starts.dequeue();

	// Tempo and Key
	file.read((char*)&output.tempo, sizeof(double));
	file.read((char*)&output.key, sizeof(double));

	// Normalization Info
	file.read((char*)&output.note_min, sizeof(unsigned int));
	file.read((char*)&output.note_max, sizeof(unsigned int));
	file.read((char*)&output.velocity_min, sizeof(unsigned int));
	file.read((char*)&output.velocity_max, sizeof(unsigned int));
	file.read((char*)&output.duration_min, sizeof(unsigned int));
	file.read((char*)&output.duration_max, sizeof(unsigned int));
	file.read((char*)&output.delay_min, sizeof(unsigned int));
	file.read((char*)&output.delay_max, sizeof(unsigned int));

	// Enqueue All Notes
	unsigned long long int size;
	file.read((char*)&size, sizeof(unsigned long long int));
	output.notes.resize(size);
	for (unsigned long long int i = 0; i < size; i++) {
		NormalizedMidiNote temp;
		file.read((char*)&temp.note, sizeof(double));
		file.read((char*)&temp.velocity, sizeof(double));
		file.read((char*)&temp.duration, sizeof(double));
		file.read((char*)&temp.delay, sizeof(double));
		output.notes.enqueue(temp);
	}

	file.read((char*)&size, sizeof(unsigned long long int));
	output.bar_starts.resize(size);
	for (unsigned long long int i = 0; i < size; i++) {
		unsigned long long int temp;
		file.read((char*)&temp, sizeof(unsigned long long int));
		output.bar_starts.enqueue(temp);
	}

	file.close();
}