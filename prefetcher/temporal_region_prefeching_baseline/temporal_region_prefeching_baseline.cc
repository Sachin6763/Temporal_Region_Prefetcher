#include <iostream>
#include <unordered_map>
#include <deque>
#include <cmath>
#include "cache.h"

#define REGION_SHIFT 12
#define MAX_HISTORY 64
#define STRIDE_HISTORY 4
#define MAX_PREFETCHES 4

std::unordered_map<uint64_t, int64_t> last_address;
std::unordered_map<uint64_t, int64_t> last_stride;
std::unordered_map<uint64_t, std::deque<int64_t>> stride_history;
std::deque<uint64_t> region_history;

int total_prefetches = 0;
int correct_prefetches = 0;

void CACHE::prefetcher_initialize() {
    std::cout << NAME << " Simple Baseline Hybrid Prefetcher initialized.\n";
    last_address.clear();
    last_stride.clear();
    stride_history.clear();
    region_history.clear();
    total_prefetches = 0;
    correct_prefetches = 0;
}

void track_region(uint64_t region) {
    if (region_history.empty() || region_history.back() != region) {
        if (region_history.size() >= MAX_HISTORY)
            region_history.pop_front();
        region_history.push_back(region);
    }
}

void stride_and_nextline(uint64_t addr, uint64_t ip, CACHE* cache) {
    // Stride prefetch
    if (last_address.count(ip)) {
        int64_t stride = last_stride[ip];
        if (stride != 0 && std::abs(stride) < (1 << 20)) {
            cache->prefetch_line(addr + stride, true, 0);
            total_prefetches++;
        }
    }

    // Update stride
    if (last_address.count(ip)) {
        int64_t stride = addr - last_address[ip];
        last_stride[ip] = stride;
        stride_history[ip].push_back(stride);
        if (stride_history[ip].size() > STRIDE_HISTORY)
            stride_history[ip].pop_front();
    }
    last_address[ip] = addr;

    // Next-line prefetch
    cache->prefetch_line(addr + (1ULL << LOG2_BLOCK_SIZE), true, 0);
    total_prefetches++;
}

uint32_t CACHE::prefetcher_cache_operate(uint64_t addr, uint64_t ip, uint8_t cache_hit,
                                         uint8_t type, uint32_t metadata_in)
{
    uint64_t region = addr >> REGION_SHIFT;
    track_region(region);

    // Temporal trigger: simple lookback match
    if (region_history.size() >= 2) {
        uint64_t prev_region = region_history[region_history.size() - 2];
        if (prev_region != region) {
            uint64_t prefetch_addr = region << REGION_SHIFT;
            for (int i = 0; i < MAX_PREFETCHES; ++i) {
                cache->prefetch_line(prefetch_addr + (i << LOG2_BLOCK_SIZE), true, 0);
                total_prefetches++;
            }
        }
    }

    // Add stride + next-line
    stride_and_nextline(addr, ip, this);

    return metadata_in;
}

uint32_t CACHE::prefetcher_cache_fill(uint64_t addr, uint32_t set, uint32_t way,
                                      uint8_t prefetch, uint64_t evicted_addr, uint32_t metadata_in)
{
    if (prefetch)
        correct_prefetches++;
    return metadata_in;
}

void CACHE::prefetcher_cycle_operate() {}

void CACHE::prefetcher_final_stats()
{
    std::cout << "Total Prefetches Issued: " << total_prefetches << std::endl;
    std::cout << "Correct Prefetches:      " << correct_prefetches << std::endl;
    if (total_prefetches > 0) {
        std::cout << "Prefetch Accuracy:       "
                  << 100.0 * correct_prefetches / total_prefetches << "%" << std::endl;
    }
}
