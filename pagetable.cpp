#include "pagetable.h"
using namespace std;

PageTable::PageTable(const vector<int>& levelBits, int numOfFrames) {
    this->levelCount = levelBits.size();
    this->numFrames = numOfFrames;

    this->bitMaskAry = vector<unsigned int>(levelCount, 0);
    this->shiftAry = vector<unsigned int>(levelCount, 0);
    this->entryCount = vector<unsigned int>(levelCount, 0);

    this->rootNodePtr = new Level(0, this);

    int totalVPN = 0;
    for (int bits : levelBits) {
        totalVPN += bits;
    }

    int offset = 32 - totalVPN;

    int currentShift = offset;

    for (int i = levelCount - 1; i >= 0; --i) {
        int bits = levelBits.at(i);
        
        this->entryCount.at(i) = 1U << bits;
        this->shiftAry.at(i) = currentShift;
        this->bitMaskAry.at(i) = (1U << bits) - 1;

        currentShift += bits;
    }
}

unsigned int PageTable::extractVPNIndex(unsigned int virtualAddress, int level) const {
    unsigned int shift = shiftAry.at(level);
    unsigned int mask = bitMaskAry.at(level);

    // Shift first, then mask.
    return (virtualAddress >> shift) & mask;
}