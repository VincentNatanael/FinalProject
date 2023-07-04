#pragma once

#include <string>
#include "Queue.hpp"

struct MidiNote {
	unsigned int note = 64;
	unsigned int velocity = 0;
	unsigned int duration = 0;
	unsigned int delay = 0;

	// The following data are only used during data creation
	unsigned long long int start = 0;
	bool finished = false;
};

struct NormalizedMidiNote {
	double note = 64;
	double velocity = 0;
	double duration = 0;
	double delay = 0;
};

struct MidiMusicalData {
	// Track Num
	unsigned int track = 0;
	// Tick
	unsigned int tick = 0;
	// Tempo
	unsigned int tempo = 0;
	bool tempo_set = false;
	// Time Signature
	unsigned char timeSignatureD = 0;
	unsigned char timeSignatureN = 0;
	unsigned char timeClockPerMetronome = 0;
	unsigned char timeNotePerClock = 0;
	bool timeSignature_set = false;
	// Key
	char key = 0;
	bool keyMinor = false;
	bool key_set = false;
	// Notes
	Queue<MidiNote> notes[16];
};

// This is not considered a C struct
struct TrainingData {
	char key = 0;
	unsigned int tempo = 0;
	unsigned int note_min = -1;
	unsigned int note_max = 0;
	unsigned int velocity_min = -1;
	unsigned int velocity_max = 0;
	unsigned int duration_min = -1;
	unsigned int duration_max = 0;
	unsigned int delay_min = -1;
	unsigned int delay_max = 0;
	Queue<MidiNote> notes;
	Queue<unsigned long long int> bar_starts;

	// The following data are only used during data normalization
	unsigned int tempo_min = -1;
	char key_min = 0x7F;
};

// This is not considered a C struct
struct NormalizedTrainingData {
	double tempo = 0;
	double key = 0;
	unsigned int note_min = -1;
	unsigned int note_max = 0;
	unsigned int velocity_min = -1;
	unsigned int velocity_max = 0;
	unsigned int duration_min = -1;
	unsigned int duration_max = 0;
	unsigned int delay_min = -1;
	unsigned int delay_max = 0;
	Queue<NormalizedMidiNote> notes;
	Queue<unsigned long long int> bar_starts;
};

void readMidiFile(MidiMusicalData*& output, const char* filename, bool debug = false);

void saveTrainingData(TrainingData& data, std::string filename);
void saveNormalizedData(NormalizedTrainingData& data, std::string filename);

void loadTrainingData(TrainingData& output, std::string filename);
void loadNormalizedData(NormalizedTrainingData& output, std::string filename);

void convertMidi(std::string filename);

void normalize(std::string list_filename);