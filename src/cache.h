/***************************************************************************/
/**
 * @file cache.h
 * @brief Header file for a write-back cache simulator with a write allocate
 * policy and a LRU replacement strategy.
 * 
 * This file defines structures and functions for simulating cache operations,
 * including calculating the number of cache hits, misses, and dirty
 * write-backs.
 ******************************************************************************/

#ifndef CACHE_H_INCLUDED
#define CACHE_H_INCLUDED

#include <stdbool.h>

/**
 * @brief Cache statistics for tracking hits, misses, and dirty write-backs.
 */
typedef struct CacheStats {
    int hits;               // Number of cache hits
    int misses;             // Number of cache misses
    int dirty_write_backs;  // Number of dirty write-backs
} CacheStats;

/**
 * @brief Cache structure representing a cache line.
 */
typedef struct CacheLine {
    int tag;        // Tag to identify a line
    bool is_valid;  // Indicates if the line contains valid data
    bool is_dirty;  // Indicates if the line has been written to
    int lru_order;  // Stores the LRU order for the line
}   CacheLine;

/**
 * @brief Cache structure representing a set (composed of multiple cache lines)
 */
typedef struct CacheSet {
    CacheLine *lines; // Array of cache lines
} CacheSet;


/**
 * @brief Main structure representing the cache itself.
 */
typedef struct Cache {
    int associativity;      // Number of lines per set (ways)
    int cache_size;         // Cache size in KB
    int line_size;          // Cache line size in bytes
    int miss_penalty;       // Penalty in cycles for a cache miss
    int dirty_wb_penalty;   // Penalty in cycles for a dirty write-back
    int num_sets;           // Number of sets in the cache
    CacheSet *sets;         // Array of cache sets
    CacheStats stats;       // Cache statistics (hits, misses, dirty write-backs)
    int log_line_size;      // Precomputed log2(line_size)
    int log_num_sets;       // Precomputed log2(num_sets)
} Cache;

/**
 * @brief Represents a cache operation (either LOAD or STORE).
 */
typedef struct CacheOp {
    char access_type;       // 'l' for LOAD, 's' for STORE
    unsigned long address;  // Address to access
    int instructions;       // Number of instructions in the operation
} CacheOp;

// --- Cache Initialization and Cleanup ---
/**
 * @brief Initializes the cache with the given configuration.
 *
 * @param associativity Cache associativity (ways per set).
 * @param cache_size Cache size in KB.
 * @param line_size Cache line size in bytes.
 * @param miss_penalty Miss penalty in cycles.
 * @param dirty_wb_penalty Dirty write-back penalty in cycles.
 * @return Pointer to the initialized Cache object
 */
Cache* initialize_cache(int associativity, int cache_size, int line_size, int miss_penalty, int dirty_wb_penalty);

/**
 * @brief Frees the memory allocated for the cache.
 *
 * @param cache Pointer to the Cache object.
 */
void free_cache(Cache *cache);

// --- Cache Access Operations ---
/**
 * @brief Initializes a cache operation with the given parameters.
 *
 * @param access_type Either 'l' for LOAD or 's' for STORE.
 * @param address A virtual address.
 * @param instructions Number of instructions performed in cache operation.
 * @return The initialized CacheOp object.
 */
CacheOp initialize_cache_operation(char access_type, unsigned long address, int instructions);

/**
 * @brief Simulates a cache access operation (LOAD or STORE).
 *
 * @param cache Pointer to the Cache object.
 * @param cache_op Pointer to the CacheOp object representing the cache operation.
 * @return `true` if the access is a hit, `false` if it's a miss.
 */
bool access_cache(Cache *cache, const CacheOp *cache_op);

// --- Cache Statistic Functions---
/**
 * @brief Returns the number of cache hits that have occurred.
 *
 * @param cache Pointer to the Cache object.
 * @return The number of cache hits.
 */
int get_cache_hits(const Cache *cache);

/**
 * @brief Returns the number of cache misses that have occurred.
 *
 * @param cache Pointer to the Cache object.
 * @return The number of cache misses.
 */
int get_cache_misses(const Cache *cache);

/**
 * @brief Returns the number of dirty write-backs that have occurred.
 *
 * @param cache Pointer to the Cache object.
 * @return The number of dirty write-backs.
 */
int get_dirty_write_backs(const Cache *cache);

#endif // CACHE_H_INCLUDED
