#include <vector>
#include <set>
#include <deque>
#include <cstdint>
#include <string>
using namespace std;


class PageTable;

class Map {
public:
    int frameNumber = -1;
    uint16_t bitstring = 0;  // 16-bit as per spec
    long lastAccessTime = 0;
    unsigned int vpn = 0;
};

class Level {
public:
    int depth;
    PageTable* rootPT;
    Level** nextLevel = nullptr;
    Map* mapArray = nullptr;

    Level(int d, PageTable* root);
    ~Level();  // Declared here; implemented in .cpp
};

class PageTable {
public:
    int levelCount;
    vector<unsigned int> bitMaskAry;
    vector<unsigned int> shiftAry;
    vector<unsigned int> entryCount;
    unsigned int entries;  // Changed to unsigned
    int nfuInterval;
    int nfuCounter = 0;
    int offset;

    Level* rootNode = nullptr;

    int numFrames;
    int framesUsed = 0;

    deque<Map*> loadedPagesCollection;
    set<Map*> accessedPagesInInterval;

    long pageHits = 0;
    long pageFaults = 0;
    long accesses = 0;
    long pageReplacements = 0;

    PageTable(const vector<int>& levelBits, int numOfFrames);
    ~PageTable() {
        delete rootNode;
    }

    unsigned int extractVPNIndex(unsigned int virtualAddress, int level) const;
    Map* searchMappedPfn(PageTable *pageTable, unsigned int virtualAddress);
    void insertMapForVpn2Pfn(PageTable *pageTable, unsigned int virtualAddress, int frame);
    void processAddress(unsigned int virtualAddress, std::string logOption);
};