
#include <iostream>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <cmath>
#include <array>
#include <deque>
#include <algorithm>

#include "cache.h"

#define REGION_SHIFT 12
#define MAX_HISTORY 1024
#define MAX_PREFETCHES 4
#define BASE_THRESHOLD 1.0
#define MAX_WEIGHT 4.0
#define MIN_WEIGHT -4.0
#define MAX_CONFIDENCE 10
#define MIN_CONFIDENCE 0
#define DECAY_INTERVAL 100
#define STRIDE_HISTORY 4
#define BANDWIDTH_LIMIT 64

struct RegionPair {
    uint64_t from;
    uint64_t to;
    bool operator==(const RegionPair& other) const {
        return from == other.from && to == other.to;
    }
};

namespace std {
    template <>
    struct hash<RegionPair> {
        std::size_t operator()(const RegionPair& rp) const {
            return hash<uint64_t>()(rp.from) ^ (hash<uint64_t>()(rp.to) << 1);
        }
    };
}

namespace HybridPrefetcher {

std::unordered_map<RegionPair, std::array<double, 3>> perceptron_weights;
std::unordered_map<uint64_t, std::vector<RegionPair>> from_region_map;
std::unordered_map<RegionPair, int> confidence_table;
std::unordered_map<RegionPair, std::unordered_set<int>> spatial_delta_map;
std::unordered_map<uint64_t, int> region_freq;
std::unordered_map<uint64_t, std::deque<int64_t>> stride_history;
std::unordered_map<uint64_t, int64_t> last_address;
std::unordered_map<uint64_t, int64_t> last_stride;
std::deque<uint64_t> region_history;
int prefetch_count; 

uint64_t cycle_counter = 0;
int correct_prefetches = 0, total_prefetches = 0;

int bandwidth_usage = 0;

void reset_internal_state() {
    perceptron_weights.clear();
    from_region_map.clear();
    region_history.clear();
    last_address.clear();
    last_stride.clear();
    stride_history.clear();
    confidence_table.clear();
    spatial_delta_map.clear();
    region_freq.clear();
    cycle_counter = correct_prefetches = total_prefetches = 0;
    bandwidth_usage = 0;
    prefetch_count = 0; 
}

void track_region(uint64_t region) {
    if (region_history.empty() || region_history.back() != region) {
        if (region_history.size() >= MAX_HISTORY)
            region_history.pop_front();
        region_history.push_back(region);
        region_freq[region]++;
    }
}

std::array<int, 3> extract_features(uint64_t ip, uint64_t addr) {
    std::array<int, 3> features = {1, 0, 0};
    if (last_address.count(ip)) {
        int64_t stride = addr - last_address[ip];
        features[1] = (stride == last_stride[ip]) ? 1 : 0;
        features[2] = (stride == (1ULL << LOG2_BLOCK_SIZE)) ? 1 : 0;
        last_stride[ip] = stride;
        stride_history[ip].push_back(stride);
        if (stride_history[ip].size() > STRIDE_HISTORY)
            stride_history[ip].pop_front();
    }
    last_address[ip] = addr;
    return features;
}

double compute_score(const std::array<double, 3>& weights, const std::array<int, 3>& features) {
    double score = 0.0;
    for (size_t i = 0; i < weights.size(); ++i)
        score += weights[i] * features[i];
    return score;
}

int sign(double score, double threshold) {
    return score >= threshold ? 1 : -1;
}

void update_weights(RegionPair& pair, const std::array<int, 3>& features, int label) {
    double lr = 0.2 * (double(confidence_table[pair]) / MAX_CONFIDENCE);
    for (size_t i = 0; i < features.size(); ++i) {
        perceptron_weights[pair][i] += label * features[i] * lr;
        perceptron_weights[pair][i] = std::clamp(perceptron_weights[pair][i], MIN_WEIGHT, MAX_WEIGHT);
    }
}

void decay_confidence() {
    for (auto& [pair, conf] : confidence_table) {
        conf = std::max(conf - 1, MIN_CONFIDENCE);
    }
    bandwidth_usage = 0;
}

void perform_prefetch(uint64_t base_addr, const std::unordered_set<int>& deltas, CACHE* cache, int confidence) {
    if (bandwidth_usage >= BANDWIDTH_LIMIT) return;
    int issued = 0;
    for (int delta : deltas) {
        if (confidence < 3 && std::abs(delta) > 64) continue;
        if (issued >= MAX_PREFETCHES) break;
        uint64_t pf_addr = base_addr + delta;
        if (delta < (1 << REGION_SHIFT)) {
            cache->prefetch_line(pf_addr, true, 0);
            total_prefetches++;
            issued++;
            prefetch_count ++; 
            bandwidth_usage++;
        }
    }
}

void stride_and_nextline(uint64_t addr, uint64_t ip, CACHE* cache) {
    
    if (prefetch_count >= MAX_PREFETCHES){
        return; 
    }

    if (last_address.count(ip)) {
        int64_t stride = last_stride[ip];
        if (stride != 0 && std::abs(stride) < (1 << 20)) {
            cache->prefetch_line(addr + stride, true, 0);
            total_prefetches++;
            prefetch_count ++ ; 
            bandwidth_usage++;
        }
    }

    if (prefetch_count >= MAX_PREFETCHES){
        return; 
    }

    cache->prefetch_line(addr + (1ULL << LOG2_BLOCK_SIZE), true, 0);
    prefetch_count ++ ; 
    total_prefetches++;
    bandwidth_usage++;
}

} // namespace HybridPrefetcher

using namespace HybridPrefetcher;

void CACHE::prefetcher_initialize() {
    std::cout << NAME << " Modular Complex Hybrid Prefetcher initialized.\n";
    reset_internal_state();
}

uint32_t CACHE::prefetcher_cache_operate(uint64_t addr, uint64_t ip, uint8_t cache_hit,
                                         uint8_t type, uint32_t metadata_in)
{
    uint64_t curr_region = addr >> REGION_SHIFT;
    cycle_counter++;
    prefetch_count = 0;
    

    track_region(curr_region);

    if (region_history.size() >= 2) {
        uint64_t prev_region = region_history[region_history.size() - 2];
        RegionPair pair{prev_region, curr_region};

        auto features = extract_features(ip, addr);

        if (perceptron_weights.find(pair) == perceptron_weights.end()) {
            perceptron_weights[pair] = {0.0, 0.0, 0.0};
            from_region_map[pair.from].push_back(pair);
            confidence_table[pair] = 1;
        }

        double score = compute_score(perceptron_weights[pair], features);
        int prediction = sign(score, BASE_THRESHOLD);
        int label = 1;

        if (prediction != label) update_weights(pair, features, label);

        confidence_table[pair] = std::min(confidence_table[pair] + 1, MAX_CONFIDENCE);
        int offset = addr & ((1ULL << REGION_SHIFT) - 1);
        spatial_delta_map[pair].insert(offset);
    }

    auto it = from_region_map.find(curr_region);
    if (it != from_region_map.end()) {
        for (const auto& pair : it->second) {
            auto& weights = perceptron_weights[pair];   
            std::array<int, 3> pf_features = {1, 0, 0};
            double score = compute_score(weights, pf_features);
            double dynamic_threshold = BASE_THRESHOLD + (1.0 - (double)confidence_table[pair] / MAX_CONFIDENCE);

            if (score >= dynamic_threshold && region_freq[curr_region] >= 2) {
                uint64_t base_addr = pair.to << REGION_SHIFT;
                perform_prefetch(base_addr, spatial_delta_map[pair], this, confidence_table[pair]);
            }
        }
    }

    stride_and_nextline(addr, ip, this);

    if (cycle_counter % DECAY_INTERVAL == 0)
        decay_confidence();

    return metadata_in;
}

uint32_t CACHE::prefetcher_cache_fill(uint64_t addr, uint32_t set, uint32_t way,
                                      uint8_t prefetch, uint64_t evicted_addr, uint32_t metadata_in)
{
    if (prefetch) correct_prefetches++;
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
