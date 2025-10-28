#include "pagetable.h"
#include <iostream>
#include "log_helpers.h"
using namespace std;

PageTable::PageTable(const vector<int>& levelBits, int numOfFrames) {
    this->levelCount = levelBits.size();
    this->numFrames = numOfFrames;

    this->bitMaskAry = vector<unsigned int>(levelCount, 0);
    this->shiftAry = vector<unsigned int>(levelCount, 0);
    this->entryCount = vector<unsigned int>(levelCount, 0);
    this->entries = 0;


    int totalVPNBits = 0;
    int currentShift = 32;
    for (int i = 0; i < levelCount; i++) {
        int bits = levelBits[i];
        currentShift -= bits;
        this->shiftAry[i] = currentShift;
        this->bitMaskAry[i] = ((1U << bits) - 1) << currentShift;
        this->entryCount[i] = (1U << bits);
        totalVPNBits += bits;
    }

    this->offset = 32 - totalVPNBits;

    this->rootNode = new Level(0, this);
}

Level::Level(int d, PageTable* root) : depth(d), rootPT(root) {
    int entries = rootPT->entryCount[d];  // number of entries at this level

    if (d < rootPT->levelCount - 1) { 
        // interior node
        nextLevel = new Level*[entries];
        for (int i = 0; i < entries; i++)
            nextLevel[i] = nullptr;
        rootPT->entries += entries;
    } else {
        // leaf node
        mapArray = new Map[entries];
        for (int i = 0; i < entries; i++)
            mapArray[i].frameNumber = -1;
        rootPT->entries += entries;
    }
}

unsigned int PageTable::extractVPNIndex(unsigned int virtualAddress, int level) const {
    return (virtualAddress & this->bitMaskAry[level]) >> this->shiftAry[level];
}

Map* PageTable::searchMappedPfn(PageTable *pageTable, unsigned int virtualAddress) {
    Level* currentLvl = pageTable->rootNode;

    for (int i = 0; i < pageTable->levelCount; i++) {
        unsigned int vpnIndex = pageTable->extractVPNIndex(virtualAddress, i);

        if(i < pageTable->levelCount - 1) { //still an interior level
            Level* nextLevel = currentLvl->nextLevel[vpnIndex];
            if(nextLevel == nullptr) { //page fault
                return nullptr;
            } else {
                currentLvl = nextLevel;
            }
        } else { //leaf node
            Map &map = currentLvl->mapArray[vpnIndex];
            if(map.frameNumber == -1) { //page fault
                return nullptr;
            } else {
                return &map;
            }
        }
    }
}

void PageTable::insertMapForVpn2Pfn (PageTable *pageTable, unsigned int virtualAddress, int frame) {
    Level* currentLvl = pageTable->rootNode;

    for (int i = 0; i < pageTable->levelCount; i++) {
        unsigned int vpnIndex = extractVPNIndex(virtualAddress, i);

        if(i < pageTable->levelCount - 1) { //still an interior level
            Level* nextLevel = currentLvl->nextLevel[vpnIndex];
            if(nextLevel == nullptr) { //page fault
                nextLevel = new Level(i + 1, pageTable);
                currentLvl->nextLevel[vpnIndex] = nextLevel;
                currentLvl = nextLevel;
            } else {
                currentLvl = nextLevel;
            }
        } else { //leaf node
            Map &map = currentLvl->mapArray[vpnIndex];
            if(map.frameNumber == -1) { //page fault
                //cout << "Inserting new map at frame " << frame << endl;
                map.frameNumber = frame;
                map.bitstring = (1U << 15);
                currentLvl->mapArray[vpnIndex] = map;
                pageTable->framesUsed++;
            } else {
                map.frameNumber = frame; //updating the frame
                map.bitstring = (1U << 15);
                currentLvl->mapArray[vpnIndex] = map;
            }
        }
    }
}

void PageTable::processAddress(unsigned int virtualAddress, string logOption) {
    
    //cout << "Generating map" << endl;
    Map* searchingMap = searchMappedPfn(this, virtualAddress);
    //cout << "Searching map" << endl;
    if(searchingMap != nullptr) {
        this->pageHits++;
        //cout << "Page hit, frame: " << searchingMap->frameNumber << endl;
        if(this->nfuCounter >= this->nfuInterval) {
            searchingMap->bitstring |= (1ULL << 15);
        }
    } else {
        //cout << "Page being inserted" << endl;
        this->pageFaults++;
        //cout << "Page missing inserting at " << this->framesUsed << endl;
        if(this->framesUsed < this->numFrames) {
            insertMapForVpn2Pfn(this, virtualAddress, this->framesUsed);
        }
    }

    if(logOption == "offset") {
        unsigned int offsetMask = (1U << this->offset) - 1;
        unsigned int offset = virtualAddress & offsetMask;
        print_num_inHex(offset);
    } else if(logOption == "vpns_pfn") {
        uint32_t vpns[this->levelCount];
        for(int i = 0; i < this->levelCount; i++) {
            vpns[i] = extractVPNIndex(virtualAddress, i);
        }
        Map* map = searchMappedPfn(this, virtualAddress);
        unsigned int pfn = (map != nullptr) ? map->frameNumber : 0;
        log_vpns_pfn(this->levelCount, vpns, pfn);
    } else if(logOption == "va2pa") {
        Map* map = searchMappedPfn(this, virtualAddress);
        unsigned int pa = (map->frameNumber << this->offset) | (virtualAddress & ((1U << this->offset) - 1));
        log_va2pa(virtualAddress, pa);
    } else if(logOption == "vpn2pfn_pr") {
        unsigned int vpn = virtualAddress >> this->offset;
        Map* map = searchMappedPfn(this, virtualAddress);
        if(map->frameNumber != -1) {
            log_mapping(vpn, map->frameNumber, -1, 0, true);
        } else {
            log_mapping(vpn, map->frameNumber, -1, 0, false);
        }
    }
}