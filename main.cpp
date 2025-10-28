#include <iostream>
#include <vector>
#include <string>
#include <cstdlib>
#include <unistd.h> 
#include <cstdio>   

#include "pagetable.h"
#include "vaddr_tracereader.h" 
#include "log_helpers.h"

// Constants for default values
#define DEFAULT_NUM_FRAMES INT32_MAX

using namespace std;

// --- Function to check if a string is a valid positive integer ---
bool isValidInteger(const string& s) {
    if (s.empty()) return false;
    for (char c : s) {
        if (!isdigit(c)) return false;
    }
    return true;
}

// --- Main Program ---
int main(int argc, char** argv) {
    
    // Default values for optional arguments
    int numFrames = DEFAULT_NUM_FRAMES;
    int maxAddresses = 0; // 0 means process all
    int nfuInterval = 10; // 10 if -b is not provided, otherwise the interval is set
    string logOption = "summary"; // defines what to log and if empty logs summary
    
    string traceFile;
    vector<int> levelBits; 

    int Option;
    // Parse command line arguments: -f (frames), -n (max addresses), -b (NFU interval), -l (logging)
    while ((Option = getopt(argc, argv, "f:n:b:l:")) != -1) {
        switch (Option) {
            case 'f': // number of frames
                numFrames = atoi(optarg);
                if (numFrames <= 0) {
                    cout << "Number of available frames must be a number and greater than 0" << endl;
                    exit(1);
                }
                break;
            case 'n': // number of address to process and cap
                maxAddresses = atoi(optarg);
                if (maxAddresses <= 0) {
                    cout << "Number of memory accesses must be a number and greater than 0" << endl;
                    exit(1);
                }
                break;
            case 'b': // the replacement interval
                nfuInterval = atoi(optarg);
                if (nfuInterval <= 0) {
                    cout << "Bit string update interval must be a number and greater than 0" << endl;
                    exit(1);
                }
                break;
            case 'l': // logging option 
                logOption = optarg;
                break;
            default:
                cout << "Error: Unknown flag encountered." << endl;
                exit(1);
        }
    }

    // --- Mandatory Arguments: Trace File and Level Bits ---
    
    int mandatoryArgStart = optind; 

    // Trace File
    if (mandatoryArgStart < argc) {
        traceFile = argv[mandatoryArgStart++];
    } else {
        exit(1);
    }

    // Level Bits (must be 1 or more)
    if (mandatoryArgStart < argc) {
        for (int i = mandatoryArgStart; i < argc; ++i) {
            string bitStr = argv[i];
            if (isValidInteger(bitStr)) {
                int bits = stoi(bitStr);
                if (bits <= 0) {
                    cout << "Level 0 page table must be at least 1 bit" << endl;
                    exit(1);
                }
                levelBits.push_back(bits);
            } else {
                cout << "Error: Invalid level bit argument: " << bitStr << endl;
                exit(1);
            }
        }
    } else {
        cout << "Error: Level bits are missing." << endl;
        exit(1);
    }
    
    // Check total bits (VPN + Offset)
    int totalBits = 0;
    for (int bits : levelBits) {
        totalBits += bits;
    }
    if (totalBits > 28) { // 32 total bits - 4 bits for offset (minimum)
        cout << "Too many bits used in page tables" << endl;
        exit(1);
    }


    // --- Simulation Setup ---

    // Initialize PageTable
    PageTable pt(levelBits, numFrames);
    pt.nfuInterval = nfuInterval;

    // Open Trace File (using C function fopen for FILE*)
    FILE* pFile = fopen(traceFile.c_str(), "r");
    if (!pFile) {
        cout << "Unable to open " << traceFile << endl;
        exit(1);
    }
    
    // Allocate Address Structure (the required C struct)
    p2AddrTr mtrace;

    // --- Simulation Loop ---

    
    while (NextAddress(pFile, &mtrace)) {
        if (maxAddresses > 0 && pt.accesses >= maxAddresses) {
            break;
        }
        pt.processAddress(mtrace.addr, logOption);
        pt.accesses++;
    }

    // --- Cleanup and Final Output ---
    fclose(pFile);
    if(logOption == "summary") {
        log_summary(1U << pt.offset, 
                    0, 
                    pt.pageHits, 
                    pt.accesses,
                    pt.framesUsed, 
                    pt.entries); 
    } else if(logOption == "bitmasks") {
        log_bitmasks(pt.levelCount, pt.bitMaskAry.data());
    }

    return 0;
}
