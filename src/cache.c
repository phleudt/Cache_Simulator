/***************************************************************************/
/**
 * @file cache.c
 * @brief Implementation of a write-back cache simulator with a write allocate
 * policy and a LRU replacement strategy.
 *
 * This source file provides the implementation for the cache simulator defined
 * in cache.h. The implementation focuses on simulating cache behavior
 * accurately to study the effects of different cache configurations on
 * performance metrics such as hit rate and the number of dirty write-backs.
 *
 * @note This simulator assumes that the cache size, line size, and
 *       associativity are all powers of two, which is a common requirement for
 *       real-world cache configurations.
 ******************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include "cache.h"

// --- Helper Functions ---

/**
 * @brief Allocates memory for the cache sets and lines.
 *
 * @param cache Pointer to the Cache object that holds the cache structure.
 */
static void allocate_cache_memory(Cache *cache) {
    cache->sets = (CacheSet *)malloc(cache->num_sets * sizeof(CacheSet));
    if (!cache->sets) {
        fprintf(stderr, "Failed to allocate memory for cache sets.\n");
        exit(EXIT_FAILURE);
    }

    // Allocate memory for each line in each set
    for (int i = 0; i < cache->num_sets; i++) {
        cache->sets[i].lines = (CacheLine *)malloc(cache->associativity * sizeof(CacheLine));
        if (!cache->sets[i].lines) {
            fprintf(stderr, "Failed to allocate memory for cache lines.\n");
            exit(EXIT_FAILURE);
        }
    }
}

/**
 * @brief Initializes the cache lines in all sets with default values.
 * Sets the tag to 0, sets valid and dirty bits to false, and assigns an LRU order.
 *
 * @param cache Pointer to the Cache object to initialize.
 */
static void initialize_cache_lines(const Cache *cache) {
    for (int i = 0; i < cache->num_sets; i++) {
        for (int j = 0; j < cache->associativity; j++) {
            CacheLine *line = &cache->sets[i].lines[j];
            line->tag = 0;
            line->is_valid = false;
            line->is_dirty = false;
            line->lru_order = j;
        }
    }
}

// --- Utility Function ---

/**
 * @brief Computes the logarithm base 2 of the given integer.
 *
 * @param n The integer input.
 * @return The logarithm base 2 of the input.
 */
static int log2(int n) {
    int log_value = 0;
    while (n >>= 1) {
        log_value++;
    }
    return log_value;
}

// --- Cache Initialization and Cleanup ---

Cache* initialize_cache(const int associativity, const int cache_size, const int line_size, const int miss_penalty,
                const int dirty_wb_penalty) {
    Cache *cache = (Cache *)malloc(sizeof(Cache));
    if (!cache) {
        fprintf(stderr, "Failed to allocate memory for cache.\n");
        exit(EXIT_FAILURE);
    }

    // Set cache parameters
    cache->cache_size = cache_size;
    cache->line_size = line_size;
    cache->miss_penalty = miss_penalty;
    cache->dirty_wb_penalty = dirty_wb_penalty;

    //Initialize cache statistics
    cache->stats = (CacheStats){0, 0, 0};

    // Configure cache as fully associative, direct-mapped, or set-associative
    if (associativity == 0) { // Fully Associative Cache
		cache->associativity = (cache_size * 1024) / line_size;
        cache->num_sets = 1;

    } else if (associativity == 1) { // Direct Mapped Cache
        cache->associativity = 1;
        cache->num_sets = (cache_size * 1024) / line_size;

    } else { // Set-Associative Cache
        cache->associativity = associativity;
        cache->num_sets = (cache_size * 1024) / (line_size * associativity);
    }

    cache->log_line_size = log2(line_size);
    cache->log_num_sets = log2(cache->num_sets);

    allocate_cache_memory(cache);
    initialize_cache_lines(cache);

    return cache;
}

void free_cache(Cache *cache) {
    for (int i = 0; i < cache->num_sets; i++) {
        free(cache->sets[i].lines); // Free line in each set
    }
    free(cache->sets); // Free the sets
    free(cache); // Free cache structure itself
}

// --- Initialize Cache Operations ---

CacheOp initialize_cache_operation(const char access_type, const unsigned long address, const int instructions) {
    CacheOp cache_op;

    // Set cache operation parameters
    cache_op.access_type = access_type;
    cache_op.address = address;
    cache_op.instructions = instructions;

    return cache_op;
}

// --- Address Extraction ---

/**
 * @brief Extracts the set index from the given memory address.
 *
 * @param address The memory address being accessed.
 * @param cache Pointer to the cache object.
 * @return The set index for the given address.
 */
static int extract_set_index(const unsigned long address, const Cache *cache) {
    if (cache->num_sets == 1) {
        return 0; // Fully associative cache, only one set
    }
    return (address >> cache->log_line_size) & ((1 << cache->log_num_sets) - 1);
}

/**
 * @brief Extracts the tag number from the given memory address.
 *
 * @param address The memory address being accessed.
 * @param cache Pointer to the Cache object.
 * @return The tag number for the given address.
 */
static int extract_tag_number(const unsigned long address, const Cache *cache) {
    return address >> (cache->log_line_size + cache->log_num_sets);
}

// --- LRU Handling ---

/**
 * @brief Updates the LRU order of the lines in a set after a hit or miss.
 * The line accessed or replaced becomes the most recently used.
 *
 * @param cache Pointer to the Cache object.
 * @param set_index The index of the set where the access occurred.
 * @param line_index The index of the cache line.
 */
static void update_lru_order(const Cache *cache, const int set_index, const int line_index) {
    CacheLine *lines = cache->sets[set_index].lines;
    const int target_order = lines[line_index].lru_order;

    // Increment the LRU order for lines with lower priority
    for (int i = 0; i < cache->associativity; i++) {
        if (lines[i].lru_order < target_order) {
            lines[i].lru_order++;
        }
    }

    // Set the accessed/replaced line as the most recently used (highest priority)
    lines[line_index].lru_order = 0;
}

/**
 * @brief Finds the index of the least recently used line in the specified set.
 * The LRU line is the one with the highest LRU order value.
 *
 * @param cache Pointer to the Cache object.
 * @param set_index Index of the set where the replacement will occur.
 * @return Index of the least recently used line in the set.
 */
static int find_lru_line_index(const Cache *cache, const int set_index) {
    const CacheLine *lines = cache->sets[set_index].lines;
    int max_order = 0;
    int lru_index = 0;

    // Find the line with the highest LRU order (least recently used)
    for (int i = 0; i < cache->associativity; i++) {
        if (!lines[i].is_valid) {
            return i; // If an invalid line was found, use it immediately
        }
        if (lines[i].lru_order > max_order) {
            max_order = lines[i].lru_order;
            lru_index = i;
        }
    }

    return lru_index;
}

// --- Access Cache ---

/**
 * @brief Checks whether a cache hit occurs in the specified set.
 * If a hit occurs, the LRU order is updated and relevant statistics are updated.
 *
 * @param cache Pointer to the Cache object.
 * @param cache_op Pointer to the CacheOp object representing the cache operation.
 * @param set_index The index of the set to check.
 * @param tag The tag value of the memory address.
 * @param cache_set Pointer to the CacheSet object representing the current cache set.
 * @return `true` if cache hit, `false` otherwise.
 */
static bool is_cache_hit(Cache *cache, const CacheOp *cache_op, const int set_index, const int tag,
                         const CacheSet *cache_set) {
    for (int i = 0; i < cache->associativity; i++) {
        CacheLine *line = &cache_set->lines[i];
        if (line->is_valid && line->tag == tag) {
            // Cache hit: update LRU and dirty bit (if needed)
            update_lru_order(cache, set_index, i);
            if (cache_op->access_type == 's') {
                line->is_dirty = true;
            }
            cache->stats.hits++;
            return true;
        }
    }
    return false;
}

/**
 * @brief Handles cache miss by replacing the least recently used line in a set and updating relevant cache statistics.
 *
 * @param cache Pointer to the Cache object.
 * @param cache_op Pointer to the CacheOp object representing the cache operation.
 * @param set_index The index of the set where the miss occurred.
 * @param tag The tag of the new memory address to store in the cache.
 * @param cache_set Pointer to the CacheSet object representing the current cache set.
 */
static void handle_cache_miss(Cache *cache, const CacheOp *cache_op, const int set_index, const int tag,
                       const CacheSet *cache_set) {
    cache->stats.misses++;
    const int lru_index = find_lru_line_index(cache, set_index);
    CacheLine *lru_line = &cache_set->lines[lru_index];

    // If the LRU line is dirty, perform a write-back
    if (lru_line->is_dirty) {
        cache->stats.dirty_write_backs++;
    }

    // Replace the LRU line with the new tag and reset flags
    lru_line->tag = tag;
    lru_line->is_valid = true;
    lru_line->is_dirty = (cache_op->access_type == 's');

    // Update LRU order after the miss
    update_lru_order(cache, set_index, lru_index);
}

bool access_cache(Cache *cache, const CacheOp *cache_op) {
    const int set_index = extract_set_index(cache_op->address, cache);

    if (set_index > cache->num_sets) {
        fprintf(stderr, "Set index is out of range");
        exit(EXIT_FAILURE);
    }

    const int tag = extract_tag_number(cache_op->address, cache);
    const CacheSet *cache_set = &cache->sets[set_index];

    // Check for cache hit
    if (is_cache_hit(cache, cache_op, set_index, tag, cache_set))
        return true;

    // Cache miss handling
    handle_cache_miss(cache, cache_op, set_index, tag, cache_set);
    return false;
}

// --- Cache Statistics ---

int get_cache_hits(const Cache *cache) {
    return cache->stats.hits;
}

int get_cache_misses(const Cache *cache) {
    return cache->stats.misses;
}

int get_dirty_write_backs(const Cache *cache) {
    return cache->stats.dirty_write_backs;
}
