// pagetable.cpp (updated)
#include "pagetable.h"
#include <iostream>
#include <limits>
#include <algorithm>
#include "log_helpers.h"
#include <climits>
using namespace std;

PageTable::PageTable(const vector<int>& levelBits, int numOfFrames) {
    levelCount = levelBits.size();
    numFrames = numOfFrames;

    bitMaskAry = vector<unsigned int>(levelCount, 0);
    shiftAry = vector<unsigned int>(levelCount, 0);
    entryCount = vector<unsigned int>(levelCount, 0);
    entries = 0;

    int totalVPNBits = 0;
    int currentShift = 32;
    for (int i = 0; i < levelCount; i++) {
        int bits = levelBits[i];
        currentShift -= bits;
        shiftAry[i] = currentShift;
        bitMaskAry[i] = ((1U << bits) - 1) << currentShift;
        entryCount[i] = (1U << bits);
        totalVPNBits += bits;
    }

    offset = 32 - totalVPNBits;
    rootNode = new Level(0, this);
}

Level::Level(int d, PageTable* root) : depth(d), rootPT(root) {
    int entries = rootPT->entryCount[d];
    if (d < rootPT->levelCount - 1) {
        nextLevel = new Level*[entries];
        for (int i = 0; i < entries; i++)
            nextLevel[i] = nullptr;
        rootPT->entries += entries;
    } else {
        mapArray = new Map[entries];
        for (int i = 0; i < entries; i++)
            mapArray[i].frameNumber = -1;
        rootPT->entries += entries;
    }
}

unsigned int PageTable::extractVPNIndex(unsigned int virtualAddress, int level) const {
    return (virtualAddress & bitMaskAry[level]) >> shiftAry[level];
}

Map* PageTable::searchMappedPfn(PageTable *pageTable, unsigned int virtualAddress) {
    Level* currentLvl = pageTable->rootNode;
    for (int i = 0; i < pageTable->levelCount; i++) {
        unsigned int vpnIndex = pageTable->extractVPNIndex(virtualAddress, i);
        if (i < pageTable->levelCount - 1) {
            Level* nextLevel = currentLvl->nextLevel[vpnIndex];
            if (!nextLevel) return nullptr;
            currentLvl = nextLevel;
        } else {
            Map &map = currentLvl->mapArray[vpnIndex];
            return (map.frameNumber == -1) ? nullptr : &map;
        }
    }
    return nullptr;
}

void PageTable::insertMapForVpn2Pfn(PageTable *pageTable, unsigned int virtualAddress, int frame) {
    Level* currentLvl = pageTable->rootNode;
    for (int i = 0; i < pageTable->levelCount; i++) {
        unsigned int vpnIndex = pageTable->extractVPNIndex(virtualAddress, i);
        if (i < pageTable->levelCount - 1) {
            if (!currentLvl->nextLevel[vpnIndex]) {
                currentLvl->nextLevel[vpnIndex] = new Level(i + 1, pageTable);
            }
            currentLvl = currentLvl->nextLevel[vpnIndex];
        } else {
            Map &map = currentLvl->mapArray[vpnIndex];
            map.frameNumber = frame;
            if (frame != -1) {
                map.bitstring = 1ULL << 15;
            }
        }
    }
}

void PageTable::processAddress(unsigned int virtualAddress, string logOption) {
    unsigned int vpn = virtualAddress >> this->offset;

    Map* map = searchMappedPfn(this, virtualAddress);

    bool aged_this_time = false;

    // Track access before aging
    if (map != nullptr && this->nfuInterval > 0) {
        this->accessedPagesInInterval.insert(map);
    }

    // NFU aging logic
    if (this->nfuInterval > 0) {
        this->nfuCounter++;
        if (this->nfuCounter >= this->nfuInterval) {
            for (Map* page : this->loadedPagesCollection) {
                page->bitstring >>= 1;
                if (this->accessedPagesInInterval.find(page) != this->accessedPagesInInterval.end()) {
                    page->bitstring |= (1ULL << 15);
                }
            }
            this->accessedPagesInInterval.clear();
            this->nfuCounter = 0;
            aged_this_time = true;
        }
    }

    if (map != nullptr) {
        this->pageHits++;
        map->lastAccessTime = this->accesses;
        if (logOption == "vpn2pfn_pr") {
            log_mapping(vpn, map->frameNumber, 0, 0, "hit");
        }
    } else {
        this->pageFaults++;
        Map* newMap;

        if (this->framesUsed < this->numFrames) {
            insertMapForVpn2Pfn(this, virtualAddress, this->framesUsed);
            newMap = searchMappedPfn(this, virtualAddress);
            newMap->vpn = vpn;
            newMap->lastAccessTime = this->accesses;
            this->loadedPagesCollection.push_back(newMap);
            this->framesUsed++;
            if (this->nfuInterval > 0 && !aged_this_time) {
                this->accessedPagesInInterval.insert(newMap);
            }
            if (logOption == "vpn2pfn_pr") {
                log_mapping(vpn, newMap->frameNumber, 0, 0, "miss");
            }
        } else {
            Map* victim = nullptr;
            unsigned long long minBits = ULLONG_MAX;
            long oldest = LONG_MAX;

            for (Map* page : this->loadedPagesCollection) {
                if (page->bitstring < minBits || (page->bitstring == minBits && page->lastAccessTime < oldest)) {
                    minBits = page->bitstring;
                    oldest = page->lastAccessTime;
                    victim = page;
                }
            }

            int reusedFrame = victim->frameNumber;
            unsigned int victimVPN = victim->vpn;
            uint16_t victimBits = static_cast<uint16_t>(victim->bitstring);

            victim->frameNumber = -1;
            this->pageReplacements++;

            insertMapForVpn2Pfn(this, virtualAddress, reusedFrame);
            newMap = searchMappedPfn(this, virtualAddress);
            newMap->vpn = vpn;
            newMap->lastAccessTime = this->accesses;

            auto it = find(this->loadedPagesCollection.begin(), this->loadedPagesCollection.end(), victim);
            if (it != this->loadedPagesCollection.end()) {
                *it = newMap;
            }

            if (this->nfuInterval > 0 && !aged_this_time) {
                this->accessedPagesInInterval.erase(victim);
                this->accessedPagesInInterval.insert(newMap);
            }

            if (logOption == "vpn2pfn_pr") {
                log_mapping(vpn, reusedFrame, victimVPN, victimBits, "miss");
            }
        }
    }

    if (logOption == "offset") {
        unsigned int offsetMask = (1U << this->offset) - 1;
        unsigned int offsetVal = virtualAddress & offsetMask;
        print_num_inHex(offsetVal);
    } else if (logOption == "vpns_pfn") {
        uint32_t vpns[this->levelCount];
        for (int i = 0; i < this->levelCount; i++) {
            vpns[i] = extractVPNIndex(virtualAddress, i);
        }
        Map* map = searchMappedPfn(this, virtualAddress);
        unsigned int pfn = (map != nullptr) ? map->frameNumber : 0;
        log_vpns_pfn(this->levelCount, vpns, pfn);
    } else if (logOption == "va2pa") {
        Map* map = searchMappedPfn(this, virtualAddress);
        unsigned int pa = (map != nullptr) ? (static_cast<unsigned int>(map->frameNumber) << this->offset) | (virtualAddress & ((1U << this->offset) - 1)) : 0;
        log_va2pa(virtualAddress, pa);
    }
}