/***************************************************************************/
/**
 * @file main.c
 * @brief Entry point for the write-back cache simulator program.
 *
 * This program simulates a write-back cache with a write allocate policy,
 * processing memory access traces to calculate and report cache performance
 * statistics, such as hit rate and the number of dirty write-backs. It accepts
 * several command-line options to configure the cache parameters and requires a
 * trace file containing the memory accesses to be simulated.
 *
 * The main process flow includes:
 * - Parsing command-line arguments to set cache configuration or display usage
 *   information.
 * - Initializing the cache with the specified or default parameters.
 * - Reading the trace file and decoding memory access operations.
 * - Simulating each memory access in the cache, recording cache hits, misses,
 *   and dirty write-backs.
 * - Calculating and displaying cache performance statistics.
 *
 * Command-line options:
 * - `-a <associativity>`: Set the cache associativity. 0 for fully associative,
 *     1 for direct mapped, and any other value for set associative.
 *     Default is 1 (direct mapped).
 * - `-l <line size>`: Set the cache line/block size in bytes.
 *     Default is 16 bytes.
 * - `-s <cache size>`: Set the total cache size in kilobytes.
 *     Default is 16 KB.
 * - `-p <miss penalty>`: Set the miss penalty in cycles for a cache miss.
 *     Default is 30 cycles.
 * - `-d <dirty write-back penalty>`: Set the penalty in cycles for writing back
 *     dirty lines.
 *     Default is 2 cycles.
 * - `<trace file>`: Specify the memory trace file to be processed.
 *
 * Usage example:
 * ```
 * ./calc -a 4 -l 32 -s 64 -p 50 -d 5 traces/gcc.trace
 * ```
 * This will run the simulator with a 4-way set associative cache, 32-byte cache
 * lines, a 64 KB cache size, a miss penalty of 50 cycles, a dirty write-back
 * penalty of 5 cycles, and process the accesses recorded in "traces/gcc.trace".
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cache.h"

/**
 * Useful default values for cache shape and cache overhead in case the
 * user doesn't overwrite them through command line arguments.
 */
enum {
	/**
	 * Fully associative if `0`, directly mapped if `1` and `n`-way associative
	 * if `n > 1`.
	 */
	ASSOCIATIVITY = 1,
	/**
	 * The size of a cache line (or block) in bytes.
	 */
	CACHE_LINE = 16,
	/**
	 * The total size of the cache in kilobytes.
	 */
	CACHE_SIZE = 16,
	/**
	 * The performance penalty of a cache miss in cycles.
	 */
	MISS_PENALTY = 30,
	/**
	 * The performance penalty of commiting a modified cache line to memory.
	 */
	DIRTY_WB_PENALTY = 2,
};

typedef struct TraceStats {
    int memory_access_count;    // Number of memory accesses
    int load_count;             // Number of load operations
    int store_count;            // Number of store operations
    int instruction_count;      // Number of instructions
    int cycle_count;            // Number of cycles
} TraceStats;

// Function Prototypes
static void printUsage(const char *prog);
static bool is_pow2(int n);
static bool validate_args(int associativity, int line_size, int cache_size, int miss_penalty, int dirty_wb_penalty);
static Cache* set_cache_configuration(int argc, const char *argv[]);
static void process_trace_line(char access_type, unsigned long address, int instructions, Cache *cache,
                               TraceStats *trace_stats);
static void simulate_cache(Cache *cache, const char *trace_file);
static void print_cache_settings(int associativity, int cache_size, int line_size, int miss_penalty, int dirty_wb_penalty);
static void print_access_stats(int memory_access_count, int load_count, int store_count);
static void print_hit_miss_stats(float miss_rate, int cache_miss_count, int cache_hit_count);
static void print_cpi_stats(int instruction_count, int cycle_count, int dirty_write_backs);

/**
 * Prints a short usage dialog to the console.
 *
 * @param prog the name of the executable
 */
static void printUsage(const char *prog) {
	printf(
		"Usage: %s [-a <assoc>] [-l <line>] [-s <size>] [-p <miss>] [-d <dirty>] <trace>\n"
		"  -a <assoc>: 0 for fully associative, 1 for direct mapped, n for n-way set associative (default: %u)\n"
		"  -l <line> : blocksize in bytes of the cache (default: %u)\n"
		"  -s <size> : size in KB of the cache (default: %u)\n"
		"  -p <miss> : miss penalty in cycles of a cache miss (default: %u)\n"
		"  -d <dirty>: penalty for writing back dirty lines (default :%u)\n"
		"  <trace>   : memory trace file\n",
		prog, ASSOCIATIVITY, CACHE_LINE, CACHE_SIZE, MISS_PENALTY, DIRTY_WB_PENALTY
	);
}

/**
 * @brief Checks if a given integer is a power of 2.
 *
 * @param n The integer to be checked.
 * @return `true` if `n` is a power of 2, `false` otherwise.
 */
static bool is_pow2(const int n) {
	return (n > 0) && (n & (n - 1)) == 0;
}

/**
 * @brief Validates cache configuration parameters.
 *
 * @param associativity Cache associativity.
 * @param line_size Cache line/block size in bytes.
 * @param cache_size  Total cache size in KB.
 * @param miss_penalty Miss penalty in cycles for a cache miss.
 * @param dirty_wb_penalty Penalty in cycles for writing back.
 * @return `true` if the validation succeeded, `false` otherwise.
 */
static bool validate_args(const int associativity, const int line_size, const int cache_size, const int miss_penalty, const int dirty_wb_penalty) {
    // Check if all arguments are positive
    if (associativity < 0 || line_size <= 0 || cache_size <= 0 || miss_penalty <= 0 || dirty_wb_penalty <= 0) {
    	fprintf(stderr, "Input arguments can't be zero (except for associativity) or less than zero.\n");
        return false;
    }
    // Check if associativity is valid: 0 (fully associative), 1 (direct mapped), or power of two
    if (associativity != 0 && associativity != 1 && !is_pow2(associativity)) {
	fprintf(stderr, "Invalid configuration for associativity: %d.\n", associativity);
	return false;
    }

    // Check if line_size and cache_size are powers of two
    if (!is_pow2(line_size) || !is_pow2(cache_size)) {
    	fprintf(stderr, "Line size and cache size must be powers of two.\n");
        return false;
    }

	 // Cache size must be at least as large as the line size
    if ((cache_size * 1024) < line_size) {
    	fprintf(stderr, "Cache size must be at least as large as the line size.\n");
        return false;
    }

    // Ensure that the cache size is divisible by the line size
    if ((cache_size * 1024) % line_size != 0) { // Convert cache_size to bytes
    	fprintf(stderr, "Cache size must be divisible by the line size.\n");
        return false;
    }
    // Ensure associativity does not exceed cache limits
    if (associativity > cache_size * 1024 / line_size) {
    	fprintf(stderr, "Associativity can't exceed the cache limits.\n");
	    return false;
    }

    return true;
}

/**
 * @brief Sets the cache configuration given input arguments.
 *
 * @param argc Number of command-line arguments.
 * @param argv Command-line arguments.
 * @return Initialized Cache object with the given constrains.
 */
static Cache *set_cache_configuration(const int argc, const char *argv[]) {
	// Set default cache parameters
	int associativity = ASSOCIATIVITY;
	int line_size = CACHE_LINE;
	int cache_size = CACHE_SIZE;
	int miss_penalty = MISS_PENALTY;
	int dirty_wb_penalty = DIRTY_WB_PENALTY;

	// Parse command line arguments
	for (int i = 1; i < argc - 1; i++) {
		if (strcmp(argv[i], "-a") == 0 && i + 1 < argc) {
			associativity = atoi(argv[++i]);
		} else if (strcmp(argv[i], "-l") == 0 && i + 1 < argc) {
			line_size = atoi(argv[++i]);
		} else if (strcmp(argv[i], "-s") == 0 && i + 1 < argc) {
			cache_size = atoi(argv[++i]);
		} else if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
			miss_penalty = atoi(argv[++i]);
		} else if (strcmp(argv[i], "-d") == 0 && i + 1 < argc) {
			dirty_wb_penalty = atoi(argv[++i]);
		} else {
			fprintf(stderr, "Invalid option or missing argument: %s.\n", argv[i]);
			printUsage(argv[0]);
			exit(EXIT_FAILURE);
		}
	}

	// Validate the input parameters
	if (!validate_args(associativity, line_size, cache_size, miss_penalty, dirty_wb_penalty)) {
		printUsage(argv[0]);
		exit(EXIT_FAILURE);
	}

	// Initialize cache with the provided configuration
	Cache *cache =  initialize_cache(associativity, cache_size, line_size, miss_penalty, dirty_wb_penalty);

    // Print cache configuration
	print_cache_settings(cache->associativity, cache->cache_size, cache->line_size, cache->miss_penalty,
	                     cache->dirty_wb_penalty);

    return cache;
}

/**
 * @brief Processes a single trace line from the trace file.
 *
 * @param access_type Type of memory access (`l` for LOAD or `s` for STORE).
 * @param address Memory address to access.
 * @param instructions Number of instructions in this trace line.
 * @param cache Pointer to the Cache object.
 * @param trace_stats Structure for saving trace statistics.
 */
static void process_trace_line(const char access_type, const unsigned long address, const int instructions,
                               Cache *cache, TraceStats *trace_stats) {
	const CacheOp *cache_op = initialize_cache_operation(access_type, address, instructions);

    // Handling cache accesses
	if (access_type == 'l') {
		trace_stats->load_count++;
	} else if (access_type == 's') {
		trace_stats->store_count++;
	} else {
		fprintf(stderr, "Unrecognized trace operation: '%c'. Skipping line.\n", access_type);
	}

	// Simulate cache access
	if (!access_cache(cache, cache_op)) {
		// If cache miss, add miss penalty to cycle count
		trace_stats->cycle_count += cache->miss_penalty;
	}

    // Update statistics
	trace_stats->instruction_count += instructions;
	trace_stats->cycle_count += instructions;
    trace_stats->memory_access_count++;
}

/**
 * @brief Simulates the cache based on the given trace file.
 *
 * @param cache Pointer to the Cache object being simulated.
 * @param trace_file The path to the trace file to be processed.
 */
static void simulate_cache(Cache *cache, const char *trace_file) {
	FILE *file = fopen(trace_file, "r");
	if (file == NULL) {
		perror("Failed to open trace file");
		exit(EXIT_FAILURE);
	}

	// Initialize cache operation variables
	char access_type;
	unsigned long address;
	int instructions;

	// Initialize trace file statistic variables
    TraceStats trace_stats = {0, 0, 0, 0, 0};

	// Read the trace line by line
	while (fscanf(file, "%c %lx %d\n", &access_type, &address, &instructions) != EOF) {
		process_trace_line(access_type, address, instructions, cache, &trace_stats);
	}

	fclose(file);

	// Fetch final statistics from the cache
	const int dirty_wb_count = get_dirty_write_backs(cache);
	const int cache_hit_count = get_cache_hits(cache);
	const int cache_miss_count = get_cache_misses(cache);

	// Adjust cycle count for dirty write-backs
	trace_stats.cycle_count += dirty_wb_count * cache->dirty_wb_penalty;

	// Calculate miss rate
	const float miss_rate = (float) cache_miss_count / trace_stats.memory_access_count;

	// Print cache statistics
	print_access_stats(trace_stats.memory_access_count, trace_stats.load_count, trace_stats.store_count);
	print_hit_miss_stats(miss_rate, cache_miss_count, cache_hit_count);
	print_cpi_stats(trace_stats.instruction_count, trace_stats.cycle_count, dirty_wb_count);
}


/**
 * @brief Prints the cache settings.
 *
 * @param associativity Cache associativity.
 * @param cache_size Cache size in kilobyte.
 * @param line_size Cache line size in bytes.
 * @param miss_penalty Miss penalty in cycles.
 * @param dirty_wb_penalty Dirty write-back penalty in cycles.
 */
static void print_cache_settings(const int associativity, const int cache_size, const int line_size, const int miss_penalty,
                          const int dirty_wb_penalty) {
	printf("CACHE SETTINGS\n");
	printf("     %s%8d\n", "Associativity:", associativity);
	printf("        %s%8d kilobyte\n", "Cache Size:", cache_size);
	printf("        %s%8d byte\n", "Block Size:", line_size);
	printf("      %s%8d cycles\n", "Miss Penalty:", miss_penalty);
	printf("  %s%8d cycles\n\n", "Dirty WB Penalty:", dirty_wb_penalty);
}

/**
 * @brief Prints the access statistics.
 *
 * @param memory_access_count Total number of memory accesses.
 * @param load_count Total number of load operations.
 * @param store_count Total number of store operations.
 */
static void print_access_stats(const int memory_access_count, const int load_count, const int store_count) {
	printf("CACHE ACCESS STATS\n");
	printf("   %s%12d\n", "Memory Accesses:", memory_access_count);
	printf("             %s%12d\n", "Loads:", load_count);
	printf("            %s%12d\n\n", "Stores:", store_count);
}

/**
 * @brief Prints the hit/miss statistics.
 *
 * @param miss_rate Cache miss rate.
 * @param cache_miss_count Total number of cache misses.
 * @param cache_hit_count Total number of cache hits.
 */
static void print_hit_miss_stats(const float miss_rate, const int cache_miss_count, const int cache_hit_count) {
	printf("CACHE HIT-MISS STATS\n");
	printf("         %s%12.5f%%\n", "Miss Rate:", miss_rate * 100); // Convert to percentage
	printf("      %s%12d\n", "Cache Misses:", cache_miss_count);
	printf("        %s%12d\n\n", "Cache Hits:", cache_hit_count);
}

/**
 * @brief Prints the CPI (Cycles Per Instruction) statistics.
 *
 * @param instruction_count Total number of instructions.
 * @param cycle_count Total number of cycles.
 * @param dirty_write_backs Total number of dirty write-backs.
 */
static void print_cpi_stats(const int instruction_count, const int cycle_count, const int dirty_write_backs) {
	printf("CACHE CPI STATS\n");
	printf("Cycles/Instruction: %11.5f\n", (float) cycle_count / instruction_count); // Show CPI with 5 decimal places
	printf("      %s%12d\n", "Instructions:", instruction_count);
	printf("            %s%12d\n", "Cycles:", cycle_count);
	printf(" %s%12d\n", "Dirty Write-Backs:", dirty_write_backs);
}

/**
 * @brief Entry point for the cache simulation program.
 *
 * @param argc Number of command-line arguments.
 * @param argv Command-line arguments.
 * @return Exit status of the program.
 */
int main(const int argc, const char *argv[]) {
	if (argc < 2) {
		printUsage(argv[0]);
		exit(EXIT_FAILURE);
	}

    // Initialize the cache based on command-line arguments
	Cache *cache = set_cache_configuration(argc, argv);

	const char *trace_file = argv[argc - 1]; // Last argument is the trace file

    // Simulate cache using the provided trace file
	simulate_cache(cache, trace_file);

    // Free allocated cache memory
	free_cache(cache);

    return EXIT_SUCCESS;
}
