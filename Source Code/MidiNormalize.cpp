#include "MidiDecoder.hpp"

#include <iostream>
#include <fstream>

void normalize(std::string list_filename) {
    // Open File
    std::ifstream list(list_filename + '\0');
    if (!list.is_open()) throw "Failed to open file";

    // Declarations
    std::string filename;
    TrainingData data;
    char key_min;
    char key_max;
    unsigned int tempo_min;
    unsigned int tempo_max;
    unsigned int note_min;
    unsigned int note_max;
    unsigned int velocity_min;
    unsigned int velocity_max;
    unsigned int duration_min;
    unsigned int duration_max;
    unsigned int delay_min;
    unsigned int delay_max;

    // Convert MIDI and Get Normalization Parameters from First Data
    getline(list, filename);
    std::cout << "Processing: " << filename << "\n";
    convertMidi(filename);
    loadTrainingData(data, filename);
    key_min = data.key;
    key_max = data.key;
    tempo_min = data.tempo;
    tempo_max = data.tempo;
    note_min = data.note_min;
    note_max = data.note_max;
    velocity_min = data.velocity_min;
    velocity_max = data.velocity_max;
    duration_min = data.duration_min;
    duration_max = data.duration_max;
    delay_min = data.delay_min;
    delay_max = data.delay_max;

    // Convert MIDI and Get Normalization Parameters
    while (getline(list, filename)) {
        std::cout << "Processing: " << filename << "\n";
        convertMidi(filename);
        loadTrainingData(data, filename);
        if (key_min > data.key) key_min = data.key;
        if (key_max < data.key) key_max = data.key;
        if (tempo_min > data.tempo) tempo_min = data.tempo;
        if (tempo_max < data.tempo) tempo_max = data.tempo;
        if (note_min > data.note_min) note_min = data.note_min;
        if (note_max < data.note_max) note_max = data.note_max;
        if (velocity_min > data.velocity_min) velocity_min = data.velocity_min;
        if (velocity_max < data.velocity_max) velocity_max = data.velocity_max;
        if (duration_min > data.duration_min) duration_min = data.duration_min;
        if (duration_max < data.duration_max) duration_max = data.duration_max;
        if (delay_min > data.delay_min) delay_min = data.delay_min;
        if (delay_max < data.delay_max) delay_max = data.delay_max;
    }

    list.close();
    list.open(list_filename + '\0');

    // Save Normalization Info
    TrainingData normalization_info;
    normalization_info.key_min = key_min;
    normalization_info.key = key_max;
    normalization_info.tempo_min = tempo_min;
    normalization_info.tempo = tempo_max;
    normalization_info.note_min = note_min;
    normalization_info.note_max = note_max;
    normalization_info.velocity_min = velocity_min;
    normalization_info.velocity_max = velocity_max;
    normalization_info.duration_min = duration_min;
    normalization_info.duration_max = duration_max;
    normalization_info.delay_min = delay_min;
    normalization_info.delay_max = delay_max;
    saveTrainingData(normalization_info, "normalization_info");

    // Normalize Data
    while (getline(list, filename)) {
        std::cout << "Normalizing: " << filename << "\n";
        loadTrainingData(data, filename);
        NormalizedTrainingData result;

        // Headers
        result.key = (double)(data.key - key_min) / (double)(key_max - key_min) * 2 - 1;
        result.tempo = (double)(data.tempo - tempo_min) / (double)(tempo_max - tempo_min) * 2 - 1;
        result.note_min = data.note_min;
        result.note_max = data.note_max;
        result.velocity_min = data.velocity_min;
        result.velocity_max = data.velocity_max;
        result.duration_min = data.duration_min;
        result.duration_max = data.duration_max;
        result.delay_min = data.delay_min;
        result.delay_max = data.delay_max;

        // Notes
        unsigned long long int size = data.notes.getSize();
        result.notes.resize(size);
        for (unsigned long long int i = 0; i < size; i++) {
            MidiNote temp = data.notes.dequeue();

            NormalizedMidiNote temp_result;
            temp_result.note = (double)(temp.note - note_min) / (double)(note_max - note_min) * 2 - 1;
            temp_result.velocity = (double)(temp.velocity - velocity_min) / (double)(velocity_max - velocity_min) * 2 - 1;
            temp_result.duration = (double)(temp.duration - duration_min) / (double)(duration_max - duration_min) * 2 - 1;
            temp_result.delay = (double)(temp.delay - delay_min) / (double)(delay_max - delay_min) * 2 - 1;

            result.notes.enqueue(temp_result);
        }

        // Bar Starts
        size = data.bar_starts.getSize();
        result.bar_starts.resize(size);
        for (unsigned long long int i = 0; i < size; i++) {
            unsigned long long int temp = data.bar_starts.dequeue();
            result.bar_starts.enqueue(temp);
        }

        saveNormalizedData(result, filename);
    }
}