#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <unistd.h>
#include <vector>
#include <cstdlib>
#include "pagetable.h"

#define DEFAULT_FRAMES 999999
#define DEFAULT_NFU_INTERVAL 50

using namespace std;

int main(int argc, char *argv[]) {
    int numOfFrames = DEFAULT_FRAMES;
    int nfuInterval = DEFAULT_NFU_INTERVAL;
    string logMode = "summary";

    int opt;
    
    while ((opt = getopt(argc, argv, "f:n:l:")) != -1) {
        switch (opt) {
            case 'f':
                // Number of physical frames
                numOfFrames = atoi(optarg);
                if (numOfFrames <= 0) {
                    cerr << "Error: Number of frames must be positive." << endl;
                    cout << argv[0];
                }
                break;
            case 'n':
                // NFU bitstring update interval
                nfuInterval = atoi(optarg);
                if (nfuInterval <= 0) {
                    cerr << "Error: NFU interval must be positive." << endl;
                    cout << argv[0];
                }
                break;
            case 'l':
                // Logging mode
                logMode = optarg;
                break;
            default:
                cout << argv[0];
        }
    }
    
    // Check if the trace file path is present (first mandatory argument)
    if (optind >= argc) {
        cerr << "Error: Missing trace file argument." << endl;
        cout << argv[0];
    }
    const char* trace_file_path = argv[optind];
    optind++;

    vector<int> level_bits;
    for (int i = optind; i < argc; ++i) {
        int bits = atoi(argv[i]);
        if (bits <= 0 || bits > 28) { 
            cerr << "Error: Invalid level bit count: " << bits << ". Must be between 1 and 28." << endl;
            cout << argv[0];
        }
        level_bits.push_back(bits);
    }

    // The sum of all level bits must be less than 32 (to leave at least 4 bits for the offset)
    int total_bits = 0;
    for(int bits : level_bits) total_bits += bits;

    if (level_bits.empty() || total_bits >= 32 || total_bits < 1) {
        cerr << "Error: Level bits not specified or sum to 32 or more (total bits: " << total_bits << ")." << endl;
        cout << argv[0];
    }

    cout << "Initializing simulation with: " << endl;
    cout << "  Frames: " << numOfFrames << endl;
    cout << "  NFU Interval: " << nfuInterval << endl;
    cout << "  Total VPN bits: " << total_bits << endl;
    cout << "  Page Offset bits: " << (32 - total_bits) << endl;
    
    // Create the main PageTable manager object
    PageTable* ptRoot = nullptr;
    try {
        ptRoot = new PageTable(level_bits, numOfFrames);
    } catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }


    ifstream trace_file(trace_file_path);
    if (!trace_file.is_open()) {
        cerr << "Error: Could not open trace file: " << trace_file_path << endl;
        return 1;
    }

    string line;
    unsigned int vAddress;
    char mode;
    long access_count = 0; // Tracks total accesses for NFU interval checking

    cout << "Processing trace file..." << endl;
    
    // Read the trace file line by line
    while (getline(trace_file, line)) {
        stringstream ss(line);
        
        // Format is: <hex_address> <mode>
        if (ss >> hex >> vAddress >> mode) {
            access_count++;
            
            // The PageTable manager handles all the logic (search, insert, NFU updates)
            //ptRoot->processAddress(vAddress, mode, nfuInterval, logMode);
        }
    }
}