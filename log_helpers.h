// log_helpers.h
#ifndef LOG_HELPERS_H
#define LOG_HELPERS_H

#include <cstdint>


/**
 * @brief Print out a number in hex, one per line
 * @param number
 */
void print_num_inHex(uint32_t number);

/**
 * @brief Print out bitmasks for all page table levels.
 *
 * @param levels - Number of levels
 * @param masks - Pointer to array of bitmasks
 */
void log_bitmasks(int levels, uint32_t *masks);

/**
 * @brief log a virtual address to physical address mapping
 * Example usages:
 *
 * @param va
 * @param pa
 */
void log_va2pa(uint32_t va, uint32_t pa);

/**
 * @brief Given a pair of numbers, output a line:
 *        src -> dest
 * Example usages:
 * log mapping between virtual and physical addresses
 *   e.g., log_mapping(va, pa, vpnReplaced, false)
 *         pagetable miss, replaced vpn
 * log mapping between vpn and pfn: mapping(page, frame)
 *   e.g., log_mapping(vpn, pfn, vpnReplaced, true)
 *         pagetable hit
 *
 * if vpnReplaced is 0, there was no page replacement
 *
 * note if vpnReplaced is bigger than 0, pthit has to be false
 *
 * @param src
 * @param dest
 * @param vpnreplaced
 * @param victim_bitstring
 * @param status
 */
void log_mapping(uint32_t src, uint32_t dest,
                 int vpnreplaced,
                 unsigned int victim_bitstring,
                 const char* status);

/**
 * @brief log vpns at all levels and the mapped physical frame number
 *
 * @param levels - specified number of levels in page table
 * @param vpns - vpns[idx] is the virtual page number associated with
 *	              level idx (0 < idx < levels)
 * @param frame - page is mapped to specified physical frame
 */
void log_vpns_pfn(int levels, uint32_t *vpns, uint32_t frame);

/**
 * @brief log summary information for the page table.
 *
 * @param page_size - Number of bytes per page
 * @param numOfPageReplaces - Number of page replacements
 * @param pageTableHits - Number of times a virtual page was mapped
 * @param numOfAddresses - Number of addresses processed
 * @param numOfFramesAllocated - Number of frames allocated
 * @param pgtableEntries - Total number of page table entries across all levels.
 */
void log_summary(unsigned int page_size,
                 unsigned int numOfPageReplaces,
                 unsigned int pageTableHits,
                 unsigned int numOfAddresses,
                 unsigned int numOfFramesAllocated,
                 unsigned long int pgtableEntries);

#endif // LOG_HELPERS_H