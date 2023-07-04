#include "MidiDecoder.hpp"

#include <fstream>

void saveTrainingData(TrainingData& data, std::string filename) {
	filename += ".bin";
	std::ofstream file(filename, std::ios::binary);

	// Tempo and Key
	file.write((char*)&data.tempo_min, sizeof(unsigned int));
	file.write((char*)&data.tempo, sizeof(unsigned int));
	file.write((char*)&data.key_min, sizeof(char));
	file.write((char*)&data.key, sizeof(char));

	// Normalization Info
	file.write((char*)&data.note_min, sizeof(unsigned int));
	file.write((char*)&data.note_max, sizeof(unsigned int));
	file.write((char*)&data.velocity_min, sizeof(unsigned int));
	file.write((char*)&data.velocity_max, sizeof(unsigned int));
	file.write((char*)&data.duration_min, sizeof(unsigned int));
	file.write((char*)&data.duration_max, sizeof(unsigned int));
	file.write((char*)&data.delay_min, sizeof(unsigned int));
	file.write((char*)&data.delay_max, sizeof(unsigned int));

	// Notes
	unsigned long long int size = data.notes.getSize();
	file.write((char*)&size, sizeof(unsigned long long int));
	for (unsigned long long int i = 0; i < size; i++) {
		MidiNote temp = data.notes.dequeue();
		file.write((char*)&temp.note, sizeof(unsigned int));
		file.write((char*)&temp.velocity, sizeof(unsigned int));
		file.write((char*)&temp.duration, sizeof(unsigned int));
		file.write((char*)&temp.delay, sizeof(unsigned int));
	}

	// Bar Starts
	size = data.bar_starts.getSize();
	file.write((char*)&size, sizeof(unsigned long long int));
	for (unsigned long long int i = 0; i < size; i++) {
		unsigned long long int temp = data.bar_starts.dequeue();
		file.write((char*)&temp, sizeof(unsigned long long int));
	}

	file.close();

	return;
}

void saveNormalizedData(NormalizedTrainingData& data, std::string filename) {
	filename += "_normalized.bin";
	std::ofstream file(filename, std::ios::binary);
	if (!file.is_open()) throw "Failed to create file";

	// Tempo and Key
	file.write((char*)&data.tempo, sizeof(double));
	file.write((char*)&data.key, sizeof(double));

	// Normalization Info
	file.write((char*)&data.note_min, sizeof(unsigned int));
	file.write((char*)&data.note_max, sizeof(unsigned int));
	file.write((char*)&data.velocity_min, sizeof(unsigned int));
	file.write((char*)&data.velocity_max, sizeof(unsigned int));
	file.write((char*)&data.duration_min, sizeof(unsigned int));
	file.write((char*)&data.duration_max, sizeof(unsigned int));
	file.write((char*)&data.delay_min, sizeof(unsigned int));
	file.write((char*)&data.delay_max, sizeof(unsigned int));

	// Notes
	unsigned long long int size = data.notes.getSize();
	file.write((char*)&size, sizeof(unsigned long long int));
	for (unsigned long long int i = 0; i < size; i++) {
		NormalizedMidiNote temp = data.notes.dequeue();
		file.write((char*)&temp.note, sizeof(double));
		file.write((char*)&temp.velocity, sizeof(double));
		file.write((char*)&temp.duration, sizeof(double));
		file.write((char*)&temp.delay, sizeof(double));
	}

	// Bar Starts
	size = data.bar_starts.getSize();
	file.write((char*)&size, sizeof(unsigned long long int));
	for (unsigned long long int i = 0; i < size; i++) {
		unsigned long long int temp = data.bar_starts.dequeue();
		file.write((char*)&temp, sizeof(unsigned long long int));
	}

	file.close();

	return;
}
