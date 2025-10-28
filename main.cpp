// main.cpp (updated)
#include <iostream>
#include <vector>
#include <string>
#include <cstdlib>
#include <cstdio>
#include <getopt.h>
#include <cctype>
#include "vaddr_tracereader.h"
#include "log_helpers.h"
#include "pagetable.h"

using namespace std;

bool isValidInteger(const string& str) {
    for (char c : str) {
        if (!isdigit(c)) return false;
    }
    return !str.empty();
}

int main(int argc, char* argv[]) {
    // Default values for optional arguments
    int numFrames = 999999; // Default infinite frames
    int maxAddresses = 0; // 0 means process all
    int nfuInterval = 10; // Default 10 if -b not provided
    string logOption;
    string traceFile;
    vector<int> levelBits;

    for (int i = 1; i < argc; ++i) {
        string arg = argv[i];

        if (arg == "-n" && i + 1 < argc) {
            maxAddresses = atoi(argv[++i]);
            if (maxAddresses < 1) {
                cout << "Number of memory accesses must be a number and greater than 0" << endl;
                return 0;
            }
        } else if (arg == "-f" && i + 1 < argc) {
            numFrames = atoi(argv[++i]);
            if (numFrames < 1) {
                cout << "Number of available frames must be a number and greater than 0" << endl;
                return 0;
            }
        } else if (arg == "-b" && i + 1 < argc) {
            nfuInterval = atoi(argv[++i]);
            if (nfuInterval < 1) {
                cout << "Bit string update interval must be a number and greater than 0" << endl;
                return 0;
            }
        } else if (arg == "-l" && i + 1 < argc) {
            logOption = argv[++i];
        } else if (arg.find(".tr") != string::npos) {
            traceFile = arg;
        } else if (isValidInteger(arg)) {
            int bits = stoi(arg);
            if (bits < 1) {
                cout << "Level " << levelBits.size() << " page table must be at least 1 bit" << endl;
                return 0;
            }
            levelBits.push_back(bits);
        } else {
        }
    }

    // Check total bits <= 28
    int totalBits = 0;
    for (int bits : levelBits) {
        totalBits += bits;
    }
    if (totalBits > 28) {
        cout << "Too many bits used in page tables" << endl;
        return 0;
    }

    // Simulation Setup
    PageTable pt(levelBits, numFrames);
    pt.nfuInterval = nfuInterval;

    if (logOption == "bitmasks") {
        log_bitmasks(pt.levelCount, pt.bitMaskAry.data());
        return 0;
    }

    // Open Trace File
    FILE* pFile = fopen(traceFile.c_str(), "r");
    if (!pFile) {
        cout << "Unable to open " << traceFile << endl;
        return 0;
    }

    p2AddrTr mtrace;

    // Simulation Loop
    while (NextAddress(pFile, &mtrace)) {
        if (maxAddresses > 0 && pt.accesses >= maxAddresses) {
            break;
        }
        pt.processAddress(mtrace.addr, logOption);
        pt.accesses++;
    }

    // Cleanup and Final Output
    fclose(pFile);
    if (logOption.empty() || logOption == "summary") {
        log_summary(1U << pt.offset,
                    pt.pageReplacements,
                    pt.pageHits,
                    pt.accesses,
                    pt.framesUsed,
                    pt.entries);
    }

    return 0;
}