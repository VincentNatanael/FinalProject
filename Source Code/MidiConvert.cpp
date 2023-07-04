#include "MidiDecoder.hpp"

#include <iostream>

void convertMidi(std::string filename) {
	// Data Initializations
	std::string filename_in = filename + ".mid";
	std::string filename_out = filename;

	MidiMusicalData* midi = nullptr;
	readMidiFile(midi, filename_in.c_str(), false);

	TrainingData output;
	output.tempo = midi[0].tempo;
	output.key = midi[0].key;
	output.bar_starts.enqueue(0);

	unsigned int track_num = midi[0].track;
	unsigned int tick = midi[0].tick;
	unsigned long long int last_start = 0;

	unsigned long long int index = 0;

	// Process Midi Data
	while (true) {
		// Get Next Played Note
		unsigned int min_trk = -1;
		unsigned int min_ch = -1;
		unsigned long long int min_start = -1;
		for (unsigned int trk = 0; trk < track_num; trk++) {
			for (unsigned int ch = 0; ch < 16; ch++) {
				if (midi[trk].notes[ch].isEmpty()) continue;
				unsigned long long int start = midi[trk].notes[ch].peek().start;
				if (start < min_start) {
					min_trk = trk;
					min_ch = ch;
					min_start = start;
				}
			}
		}

		// Break Loop if There is No Next Note
		if (min_trk == -1 && min_ch == -1) break;

		// Process Next Note
		if (true) {
			// Get Next Note
			MidiNote nextNote = midi[min_trk].notes[min_ch].dequeue();
			nextNote.delay = (unsigned int)(nextNote.start - last_start);
			last_start = nextNote.start;

			// Transpose to C
			nextNote.note -= output.key;

			// Get Data Normalization Parameters
			if (nextNote.note < output.note_min) output.note_min = nextNote.note;
			if (nextNote.note > output.note_max) output.note_max = nextNote.note;
			if (nextNote.velocity < output.velocity_min) output.velocity_min = nextNote.velocity;
			if (nextNote.velocity > output.velocity_max) output.velocity_max = nextNote.velocity;
			if (nextNote.duration < output.duration_min) output.duration_min = nextNote.duration;
			if (nextNote.duration > output.duration_max) output.duration_max = nextNote.duration;
			if (nextNote.delay < output.delay_min) output.delay_min = nextNote.delay;
			if (nextNote.delay > output.delay_max) output.delay_max = nextNote.delay;

			// Check if Note is at Bar Start
			if (nextNote.delay != 0 && (nextNote.start % (tick*4) == 0)) {
				output.bar_starts.enqueue(index);
			}

			// Enqueue Note to Output
			output.notes.enqueue(nextNote);

			index++;
		}
	}

	// Saving Training Data
	saveTrainingData(output, filename_out);

	delete[] midi;
	midi = nullptr;

	return;
}