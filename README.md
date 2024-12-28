# Cache Calculator

## Overview
This cache calculator implements a write-back cache with a write-allocate policy, designed to simulate and analyze cache behavior for different configurations. It calculates various performance metrics, such as the number of cache hits, misses, and dirty write-backs, allowing users to study the impact of cache configurations on performance.

## Features
- Simulates a write-back cache with write-allocate policy and LRU replacement strategy.
- Configurable cache size, line size, and associativity.
- Calculates performance metrics including hit rate, misses, and dirty write-backs.
- Supports loading from trace files containing memory access patterns.

## Running the Program
To run the cache calculator, use the following command line syntax:
```console
$ ./calc [-a <associativity>] [-l <line size>] [-s <cache size>] [-p <miss penalty>] [-d <dirty wb penalty>] <trace file>
```
- `-a <associativity>`: Set the cache's associativity. Default is 1 (direct-mapped).
- `-l <line size>`: Set the cache line size in bytes. Default is 16 bytes.
- `-s <cache size>`: Set the total cache size in kilobytes. Default is 16 KB.
- `-p <miss penalty>`: Set the penalty for a cache miss in cycles. Default is 30 cycles.
- `-d <dirty wb penalty>`: Set the penalty for writing back a dirty line in cycles. Default is 2 cycles.
- `<trace file>`: Path to the memory access trace file.

## Example Usage
```console
$ ./calc -a 4 -l 32 -s 64 -p 50 -d 5 traces/gcc.trace
CACHE SETTINGS
     Associativity:    4
        Cache Size:   64 kilobyte
        Block Size:   32 byte
      Miss Penalty:   50 cycles
  Dirty WB Penalty:    5 cycles

CACHE ACCESS STATS
   Memory Accesses:   515683
             Loads:   318197
            Stores:   197486

CACHE HIT-MISS STATS
         Miss Rate:  1.23681%
      Cache Misses:     6378
        Cache Hits:   509305

CACHE CPI STATS
Cycles/Instruction:  1.32827
      Instructions:  1024481
            Cycles:  1360786
 Dirty Write-Backs:     3481
```
