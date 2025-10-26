#include <vector>
using namespace std;
class PageTable;

// -------------------------------------- The Map Class --------------------------------------
class Map {
public:
    int frameNumber = -1; // negative -1 means it is not valid and any positive number means valid
    unsigned long long bitstring = 0; 
};
// -------------------------------------- The Map Class --------------------------------------



// -------------------------------------- The Level Class --------------------------------------
class Level {
public:
    int depth;
    PageTable* rootPT;

    Level** nextLevel = nullptr;
    Map* mapArray = nullptr;

    Level(int d, PageTable* root);
};
// -------------------------------------- The Level Class --------------------------------------



// -------------------------------------- The PageTable Class  --------------------------------------
class PageTable {
public:
    int levelCount;
    vector<unsigned int> bitMaskAry;
    vector<unsigned int> shiftAry;
    vector<unsigned int> entryCount;
    int entries;
    int nfuInterval;
    int nfuCounter = 0;
    int offset;

    Level* rootNode = nullptr;

    int numFrames;
    int framesUsed = 0;
    
    vector<Map*> loadedPagesCollection; 

    // Statistics
    long pageHits = 0;
    long pageFaults = 0;
    long accesses = 0;

    // Constructor to handle initialization and calculations
    PageTable(const vector<int>& levelBits, int numOfFrames);

    unsigned int extractVPNIndex(unsigned int virtualAddress, int level) const;
    Map* searchMappedPfn(PageTable *pageTable, unsigned int virtualAddress);
    unsigned int extractVPNFromVirtualAddress(unsigned int virtualAddress, unsigned int mask, unsigned int shift);
    void insertMapForVpn2Pfn (PageTable *pageTable, unsigned int virtualAddress, int frame);
    void processAddress(unsigned int virtualAddress, string logOption);
};
// -------------------------------------- The PageTable Class  --------------------------------------