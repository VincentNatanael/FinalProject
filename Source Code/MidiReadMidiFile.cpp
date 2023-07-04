#include "MidiDecoder.hpp"

#include <iostream>
#include <fstream>

void readMidiFile(MidiMusicalData*& output, const char* filename, bool debug) {
	// Initialize Header Data
	unsigned int format = 0;
	unsigned int tracks = 0;
	unsigned int ticks = 0;
	int frames = 0;
	bool smtpe_division = false;

	std::ifstream file(filename, std::ios::binary);
	if (!file.is_open()) throw "Failed to open file";

	// Initialize Chunk
	char headerType[5] = { 0,0,0,0,0 };
	unsigned int headerLength = 0;
	unsigned char* h_data = nullptr;

	// Get Header
	do {
		if (h_data != nullptr) delete[] h_data;
		// Get chunk type
		file.read(headerType, 4);
		// Get chunk length
		for (unsigned int i = 0; i < 4; i++) {
			unsigned char byte = 0;
			headerLength <<= 8;
			file.read((char*)&byte, 1);
			headerLength += byte;
		}
		// Get data
		h_data = new unsigned char[headerLength];
		for (unsigned int i = 0; i < headerLength; i++) {
			file.read((char*)&h_data[i], 1);
		}
		if (debug) std::cout << "\nReading Chunk: " << headerType << "\n";
	} while (strcmp(headerType, "MThd") != 0);

	// Read Header Data
	if (true) {
		unsigned int i = 0;
		// Read MIDI Format
		format = h_data[i++];
		format <<= 8;
		format += h_data[i++];
		if (debug) std::cout << "  MIDI Format: " << format << "\n";
		// Read Number of Tracks
		tracks = h_data[i++];
		tracks <<= 8;
		tracks += h_data[i++];
		if (debug) std::cout << "  Num of Tracks: " << tracks << "\n";
		// Read Time Divisions
		// Division by ticks per quarter note
		if ((h_data[i] & 0b10000000) == 0) {
			smtpe_division = false;
			ticks = h_data[i++];
			ticks <<= 8;
			ticks += h_data[i++];
			frames = 0;
			if (debug) std::cout << "  Ticks per Quarter Note: " << ticks << "\n";
		}
		// Division by ticks per SMTPE frames and SMTPE frames per second
		else {
			smtpe_division = true;
			ticks = h_data[i++] & 0b01111111;
			frames = -h_data[i++];
			if (debug) std::cout << "  Ticks per SMTPE Frame: " << ticks << "\n";
			if (debug) std::cout << "  SMTPE Frames per Sec: " << frames << "\n";
		}
	}
	delete[] h_data;

	// Integrity Check
	if (true) {
		if (format != 1) throw "MIDI format not supported";
		if (smtpe_division) throw "Division by SMTPE frames not supported";
		if (debug) std::cout << "  Integrity Check Passed\n";
	}

	// Midi Musical Data
	output = new MidiMusicalData[tracks];
	for (unsigned int i = 0; i < tracks; i++) {
		output[i].track = tracks;
		output[i].tick = ticks;
	}

	// For every track,
	for (unsigned int track = 0; track < tracks; track++) {
		// Initialize Track Data
		char chunkType[5] = { 0,0,0,0,0 };
		unsigned int chunkLength = 0;
		unsigned char* data = nullptr;

		// Get Track
		do {
			if (data != nullptr) delete[] data;
			// Get chunk type
			file.read(chunkType, 4);
			// Get chunk length
			chunkLength = 0;
			for (unsigned int i = 0; i < 4; i++) {
				unsigned char byte = 0;
				chunkLength <<= 8;
				file.read((char*)&byte, 1);
				chunkLength += byte;
			}
			// Get data
			data = new unsigned char[chunkLength];
			for (unsigned int i = 0; i < chunkLength; i++) {
				file.read((char*)&data[i], 1);
			}
			if (debug) std::cout << "\n\nReading Chunk: " << chunkType << "";
		} while (strcmp(chunkType, "MTrk") != 0);

		// Track Data Initialization
		unsigned int i = 0;
		unsigned char prevStatus;
		unsigned long long int sigmaDelta = 0;

		// Read Track Data
		while (i < chunkLength) {
			// Get Delta
			unsigned int delta = 0;
			unsigned char byte = 0;
			do {
				byte = data[i++];
				delta <<= 7;
				delta += byte & 0x7F;
			} while ((byte & 0x80) == 0x80);
			sigmaDelta += delta;

			// Get Status Byte
			unsigned char statusByte = data[i++];
			if ((statusByte & 0x80) == 0x80) {
				if (debug) std::cout << "\n\nNew Status Byte: " << (int)statusByte << " (byte " << i << ")\n";
				prevStatus = statusByte;
			}
			else {
				statusByte = prevStatus;
				i--;
			}

			// Get Channel (or System Event Type)
			unsigned char channel = statusByte & 0x0F;

			// Print Debug Messages
			if (debug) std::cout << "  Delta: " << (int)delta << "\n";

			// Read and Process Events
			switch (statusByte & 0xF0) {
			case 0x80: // Note Off
				if (true) {
					unsigned char note = data[i++]; // Note
					unsigned char velocity = data[i++]; // Velocity
					// Print debug messages
					if (debug) std::cout << "    Note Off ch" << ((int)channel) + 1 << "\n";
					if (debug) std::cout << "    Note: " << (int)note << "\n";
					if (debug) std::cout << "    Velocity: " << (int)velocity << "\n";
					// Make the note finished if event channel and note is the same
					for (unsigned int ch = 0; ch < 16; ch++) {
						for (unsigned int i = 0; i < output[track].notes[ch].getSize(); i++) {
							MidiNote temp = output[track].notes[ch].dequeue();
							if (!temp.finished) {
								if (ch == channel && temp.note == note) {
									temp.finished = true;
									temp.duration = (unsigned int)(sigmaDelta - temp.start);
								}
							}
							output[track].notes[ch].enqueue(temp);
						}
					}
				}
				break;
			case 0x90: // Note On
				if (true) {
					unsigned char note = data[i++]; // Note
					unsigned char velocity = data[i++]; // Velocity
					// Print debug messages
					if (debug) std::cout << "    Note On ch" << ((int)channel) + 1 << "\n";
					if (debug) std::cout << "    Note: " << (int)note << "\n";
					if (debug) std::cout << "    Velocity: " << (int)velocity << "\n";
					// Note On event with Velocity 0 is treated as Note Off event
					if (velocity != 0) {
						// Process Note On event
						output[track].notes[channel].enqueue(MidiNote{ note, velocity, 0, delta, sigmaDelta });
					}
					else {
						// Make the note finished if event channel and note is the same
						for (unsigned int ch = 0; ch < 16; ch++) {
							for (unsigned int i = 0; i < output[track].notes[ch].getSize(); i++) {
								MidiNote temp = output[track].notes[ch].dequeue();
								if (!temp.finished) {
									if (ch == channel && temp.note == note) {
										temp.finished = true;
										temp.duration = (unsigned int)(sigmaDelta - temp.start);
									}
								}
								output[track].notes[ch].enqueue(temp);
							}
						}
					}
				}
				break;
			case 0xA0: // Polyphonic Aftertouch
				if (true) {
					unsigned char note = data[i++]; // Note
					unsigned char pressure = data[i++]; // Pressure
					if (debug) std::cout << "    Note: " << (int)note << "\n";
					if (debug) std::cout << "    Velocity: " << (int)pressure << "\n";
				}
				break;
			case 0xB0: // Control/Mode Change
				if (true) {
					unsigned char controlFunction = data[i++];
					unsigned char controlValue = data[i++];
					if (debug) std::cout << "    Control Change #" << (int)controlFunction << "\n";
					if (debug) std::cout << "    Control Value: " << (int)controlValue << "\n";
				}
				break;
			case 0xC0: // Program Change
				if (true) {
					unsigned char programNumber = data[i++];
					if (debug) std::cout << "    Program change to " << (int)programNumber << "\n";
				}
				break;
			case 0xD0: // Channel Aftertouch
				if (true) {
					unsigned char pressure = data[i++]; // Pressure
				}
				break;
			case 0xE0: // Pitch Bend Change
				if (true) {
					unsigned char lsb = data[i++]; // Least Significant Byte
					unsigned char msb = data[i++]; // Most Significant Byte
				}
				break;
			case 0xF0: // System Event
				if (true) {
					// Read and Process System Event if it is a Meta Event
					if (channel == 0x0F) {
						// Get event type
						unsigned char eventType = data[i++];
						// Get event length
						unsigned char byte = 0;
						unsigned int eventLength = 0;
						do {
							byte = data[i++];
							eventLength <<= 7;
							eventLength += byte & 0x7F;
						} while ((byte & 0x80) == 0x80);
						// Read event
						unsigned char* eventData = new unsigned char[eventLength];
						for (unsigned int v = 0; v < eventLength; v++) {
							eventData[v] = data[i++];
						}
						// Check event type
						switch (eventType) {
						case 0x2F: // End of track
							i = chunkLength;
							if (debug) std::cout << "  End of Track\n";
							break;
						case 0x51: // Set tempo
							if (true) {
								unsigned int tempValue = 0;
								for (unsigned int v = 0; v < eventLength; v++) {
									tempValue <<= 8;
									tempValue += eventData[v];
								}
								if (output[track].tempo == 0) {
									output[track].tempo = tempValue;
								}
								if (debug) std::cout << "  Chage tempo to " << tempValue << "\n";
							}
							break;
						case 0x58: // Set time signature
							if (!output[track].timeSignature_set) {
								output[track].timeSignatureD = eventData[0];
								output[track].timeSignatureN = 1 << eventData[1];
								output[track].timeClockPerMetronome = eventData[2];
								output[track].timeNotePerClock = eventData[3];
							}
							if (debug) std::cout << "  Change time signature to " << (int)eventData[0] << "/" << (int)(1 << eventData[1]) << "\n";
							if (debug) std::cout << "    Midi Clock/Tick: " << (int)eventData[2] << "\n";
							if (debug) std::cout << "    1/32th Note/Midi Clock: " << (int)eventData[3] << "\n";
							break;
						case 0x59: // Set key signature
							if (!output[track].key_set) {
								switch ((char)eventData[0]) {
									// No sharps/flats
								default: output[track].key =  0; break; // C if invalid key
								case  0: output[track].key =  0; break; // C
								// Sharp keys
								case  1: output[track].key =  7; break; // G
								case  2: output[track].key =  2; break; // D
								case  3: output[track].key =  9; break; // A
								case  4: output[track].key =  4; break; // E
								case  5: output[track].key = 11; break; // B
								case  6: output[track].key =  6; break; // F#
								case  7: output[track].key =  1; break; // C#
								// Flat keys
								case -1: output[track].key =  5; break; // F
								case -2: output[track].key = 10; break; // Bb
								case -3: output[track].key =  3; break; // Eb
								case -4: output[track].key =  8; break; // Ab
								case -5: output[track].key =  1; break; // Db
								case -6: output[track].key =  6; break; // Gb
								case -7: output[track].key = 11; break; // Cb
								}
								if (output[track].key >= 6) output[track].key -= 12;
								output[track].keyMinor = eventData[1] == 1;
							}
							if (debug) std::cout << "  Change Key Signature to " << (((int)eventData[0] >= 0) ? "+" : "") << output[track].key << " " << (((int)eventData[1] == 1) ? "Minor" : "Major") << "\n";
							break;
						default: // Ignore any other meta events
							if (debug) std::cout << "  Meta Event: " << (int)eventType << "\n";
							if (debug) std::cout << "  Length: " << (int)eventLength << "\n";
							break;
						}
						break;
					}
					// Other System Event is Not Supported
					else {
						if (debug) std::cout << "** System Event is Not Supported **\n";
						throw "System Event is Not Supported";
					}
				}
				break;
			default: // Should not be possible unless there is an error in reading the track
				throw "Error in reading MIDI Track Chunk";
				break;
			}
		}
	}

	file.close();
}