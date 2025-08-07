#pragma once

#include <algorithm>
#include <execution>
#include <thread>
#include <vector>
#include <cstring>
#include <concurrentqueue/moodycamel/concurrentqueue.h>
#include <immintrin.h>
#include <cmath>
#include "item.h"

namespace optimized_sort {

// Thread count control
inline thread_local unsigned int g_thread_count = std::thread::hardware_concurrency();

inline void set_thread_count(unsigned int count) {
    g_thread_count = count > 0 ? count : std::thread::hardware_concurrency();
}

// Radix sort for integer-based sorting (extremely fast for item lengths)
class RadixSort {
public:
    static void sort_by_length(std::vector<item>& items, bool ascending = true) {
        if (items.size() < 2) return;

        // Find max length for number of passes
        int max_length = 0;
        for (const auto& item : items) {
            max_length = std::max(max_length, item.get_length());
        }

        // Temporary buffer - use reserve instead of resize
        std::vector<item> buffer;
        buffer.reserve(items.size());

        // Number of bits to sort at a time (8 bits = 1 byte)
        constexpr int RADIX_BITS = 8;
        constexpr int RADIX_SIZE = 1 << RADIX_BITS;
        constexpr int RADIX_MASK = RADIX_SIZE - 1;

        // Count array for each radix
        std::vector<size_t> count(RADIX_SIZE);
        std::vector<size_t> prefix(RADIX_SIZE);

        // Process each byte
        for (int shift = 0; shift < 32 && (max_length >> shift) > 0; shift += RADIX_BITS) {
            // Clear counts
            std::fill(count.begin(), count.end(), 0);

            // Count occurrences
            for (const auto& item : items) {
                int bucket = (item.get_length() >> shift) & RADIX_MASK;
                count[bucket]++;
            }

            // Compute positions
            prefix[0] = 0;
            if (ascending) {
                for (int i = 1; i < RADIX_SIZE; i++) {
                    prefix[i] = prefix[i - 1] + count[i - 1];
                }
            } else {
                // For descending, reverse the prefix sum
                prefix[RADIX_SIZE - 1] = 0;
                for (int i = RADIX_SIZE - 2; i >= 0; i--) {
                    prefix[i] = prefix[i + 1] + count[i + 1];
                }
            }

            // Clear buffer and build output array
            buffer.clear();
            buffer.resize(items.size(), item(0, 0, 0, 0.0)); // Initialize with dummy items

            // Reset prefix array for actual placement
            prefix[0] = 0;
            if (ascending) {
                for (int i = 1; i < RADIX_SIZE; i++) {
                    prefix[i] = prefix[i - 1] + count[i - 1];
                }
            } else {
                prefix[RADIX_SIZE - 1] = 0;
                for (int i = RADIX_SIZE - 2; i >= 0; i--) {
                    prefix[i] = prefix[i + 1] + count[i + 1];
                }
            }

            // Distribute items
            for (auto& item : items) {
                int bucket = (item.get_length() >> shift) & RADIX_MASK;
                buffer[prefix[bucket]++] = std::move(item);
            }

            // Swap back
            items.swap(buffer);
        }
    }
};

// Parallel Radix sort for integer-based sorting
class ParallelRadixSort {
public:
    static void sort_by_length(std::vector<item>& items, bool ascending = true) {
        if (items.size() < 2) return;

        const size_t num_threads = g_thread_count;
        const size_t min_items_per_thread = 10000;

        // Use serial version for small datasets
        if (items.size() < min_items_per_thread * 2) {
            RadixSort::sort_by_length(items, ascending);
            return;
        }

        // Find max length in parallel
        std::vector<int> max_lengths(num_threads, 0);
        std::vector<std::thread> threads;
        const size_t chunk_size = items.size() / num_threads;

        for (size_t t = 0; t < num_threads; ++t) {
            threads.emplace_back([&, t]() {
                size_t start = t * chunk_size;
                size_t end = (t == num_threads - 1) ? items.size() : (t + 1) * chunk_size;
                int local_max = 0;
                for (size_t i = start; i < end; ++i) {
                    local_max = std::max(local_max, items[i].get_length());
                }
                max_lengths[t] = local_max;
            });
        }

        for (auto& thread : threads) {
            thread.join();
        }

        int max_length = *std::max_element(max_lengths.begin(), max_lengths.end());

        // Parallel radix sort
        constexpr int RADIX_BITS = 8;
        constexpr int RADIX_SIZE = 1 << RADIX_BITS;
        constexpr int RADIX_MASK = RADIX_SIZE - 1;

        std::vector<item> buffer(items.size(), item(0, 0, 0, 0.0));

        for (int shift = 0; shift < 32 && (max_length >> shift) > 0; shift += RADIX_BITS) {
            // Parallel counting phase
            std::vector<std::vector<size_t>> thread_counts(num_threads, std::vector<size_t>(RADIX_SIZE, 0));
            threads.clear();

            for (size_t t = 0; t < num_threads; ++t) {
                threads.emplace_back([&, t]() {
                    size_t start = t * chunk_size;
                    size_t end = (t == num_threads - 1) ? items.size() : (t + 1) * chunk_size;
                    for (size_t i = start; i < end; ++i) {
                        int bucket = (items[i].get_length() >> shift) & RADIX_MASK;
                        thread_counts[t][bucket]++;
                    }
                });
            }

            for (auto& thread : threads) {
                thread.join();
            }

            // Merge counts and compute global positions
            std::vector<size_t> global_count(RADIX_SIZE, 0);
            for (const auto& tc : thread_counts) {
                for (int i = 0; i < RADIX_SIZE; ++i) {
                    global_count[i] += tc[i];
                }
            }

            std::vector<size_t> prefix(RADIX_SIZE);
            prefix[0] = 0;
            if (ascending) {
                for (int i = 1; i < RADIX_SIZE; i++) {
                    prefix[i] = prefix[i - 1] + global_count[i - 1];
                }
            } else {
                prefix[RADIX_SIZE - 1] = 0;
                for (int i = RADIX_SIZE - 2; i >= 0; i--) {
                    prefix[i] = prefix[i + 1] + global_count[i + 1];
                }
            }

            // Parallel distribution phase
            std::vector<std::atomic<size_t>> atomic_prefix(RADIX_SIZE);
            for (int i = 0; i < RADIX_SIZE; ++i) {
                atomic_prefix[i].store(prefix[i]);
            }

            threads.clear();
            for (size_t t = 0; t < num_threads; ++t) {
                threads.emplace_back([&, t]() {
                    size_t start = t * chunk_size;
                    size_t end = (t == num_threads - 1) ? items.size() : (t + 1) * chunk_size;
                    for (size_t i = start; i < end; ++i) {
                        int bucket = (items[i].get_length() >> shift) & RADIX_MASK;
                        size_t pos = atomic_prefix[bucket].fetch_add(1);
                        buffer[pos] = items[i];
                    }
                });
            }

            for (auto& thread : threads) {
                thread.join();
            }

            items.swap(buffer);
        }
    }
};

// Parallel merge sort for large datasets
class ParallelMergeSort {
private:
    static constexpr size_t PARALLEL_THRESHOLD = 100'000;

    template<typename Iterator, typename Compare>
    static void merge(Iterator first, Iterator mid, Iterator last,
                      std::vector<typename std::iterator_traits<Iterator>::value_type>& buffer,
                      Compare comp) {
        auto left = first;
        auto right = mid;
        buffer.clear();
        buffer.reserve(std::distance(first, last));

        while (left != mid && right != last) {
            if (comp(*left, *right)) {
                buffer.push_back(std::move(*left++));
            } else {
                buffer.push_back(std::move(*right++));
            }
        }

        while (left != mid) {
            buffer.push_back(std::move(*left++));
        }

        while (right != last) {
            buffer.push_back(std::move(*right++));
        }

        std::move(buffer.begin(), buffer.end(), first);
    }

    template<typename Iterator, typename Compare>
    static void parallel_merge_sort_impl(Iterator first, Iterator last,
                                         std::vector<typename std::iterator_traits<Iterator>::value_type>& buffer,
                                         Compare comp, size_t depth = 0) {
        size_t size = std::distance(first, last);

        if (size < 2) return;

        // Use std::sort for small sizes
        if (size < 1000) {
            std::sort(first, last, comp);
            return;
        }

        Iterator mid = first + size / 2;

        // Decide whether to parallelize based on size and depth
        if (size > PARALLEL_THRESHOLD && depth < 4) {
            // Create separate buffer for left thread
            std::vector<typename std::iterator_traits<Iterator>::value_type> left_buffer;

            // Parallel execution
            std::thread left_thread([&]() {
                parallel_merge_sort_impl(first, mid, left_buffer, comp, depth + 1);
            });

            parallel_merge_sort_impl(mid, last, buffer, comp, depth + 1);

            left_thread.join();
        } else {
            // Sequential execution
            parallel_merge_sort_impl(first, mid, buffer, comp, depth + 1);
            parallel_merge_sort_impl(mid, last, buffer, comp, depth + 1);
        }

        merge(first, mid, last, buffer, comp);
    }

public:
    template<typename Iterator, typename Compare>
    static void sort(Iterator first, Iterator last, Compare comp) {
        size_t size = std::distance(first, last);
        if (size < 2) return;

        // Allocate temporary buffer
        using ValueType = typename std::iterator_traits<Iterator>::value_type;
        std::vector<ValueType> buffer;
        buffer.reserve(size);

        parallel_merge_sort_impl(first, last, buffer, comp);
    }

    static void sort_by_length(std::vector<item>& items, bool ascending = true) {
        if (ascending) {
            sort(items.begin(), items.end(),
                 [](const item& a, const item& b) { return a.get_length() < b.get_length(); });
        } else {
            sort(items.begin(), items.end(),
                 [](const item& a, const item& b) { return a.get_length() > b.get_length(); });
        }
    }
};

// Intel's Parallel STL (if available)
class ParallelSTLSort {
public:
    static void sort_by_length(std::vector<item>& items, bool ascending = true) {
        if (ascending) {
            std::sort(std::execution::par_unseq, items.begin(), items.end(),
                      [](const item& a, const item& b) { return a.get_length() < b.get_length(); });
        } else {
            std::sort(std::execution::par_unseq, items.begin(), items.end(),
                      [](const item& a, const item& b) { return a.get_length() > b.get_length(); });
        }
    }
};

// Counting sort for limited range of lengths
class CountingSort {
public:
    static void sort_by_length(std::vector<item>& items, bool ascending = true) {
        if (items.size() < 2) return;

        // Find min and max lengths
        int min_length = std::numeric_limits<int>::max();
        int max_length = 0;
        for (const auto& item : items) {
            min_length = std::min(min_length, item.get_length());
            max_length = std::max(max_length, item.get_length());
        }

        // Use counting sort only if range is reasonable
        int range = max_length - min_length + 1;
        if (range > 1'000'000) {
            // Fall back to std::sort for large ranges
            if (ascending) {
                std::sort(items.begin(), items.end());
            } else {
                std::sort(items.begin(), items.end(), std::greater<item>());
            }
            return;
        }

        // Count occurrences of each length
        std::vector<std::vector<item>> buckets(range);

        // Distribute items into buckets
        for (auto& item : items) {
            int bucket_idx = item.get_length() - min_length;
            buckets[bucket_idx].push_back(std::move(item));
        }

        // Collect items back
        items.clear();
        if (ascending) {
            for (auto& bucket : buckets) {
                for (auto& item : bucket) {
                    items.push_back(std::move(item));
                }
            }
        } else {
            for (int i = range - 1; i >= 0; i--) {
                for (auto& item : buckets[i]) {
                    items.push_back(std::move(item));
                }
            }
        }
    }
};

// Parallel Counting Sort
class ParallelCountingSort {
public:
    static void sort_by_length(std::vector<item>& items, bool ascending = true) {
        if (items.size() < 2) return;

        const size_t num_threads = g_thread_count;
        const size_t min_items_per_thread = 10000;

        // Use serial version for small datasets
        if (items.size() < min_items_per_thread * 2) {
            CountingSort::sort_by_length(items, ascending);
            return;
        }

        // Find min/max in parallel
        std::vector<std::pair<int, int>> thread_ranges(num_threads, {INT_MAX, 0});
        std::vector<std::thread> threads;
        const size_t chunk_size = items.size() / num_threads;

        for (size_t t = 0; t < num_threads; ++t) {
            threads.emplace_back([&, t]() {
                size_t start = t * chunk_size;
                size_t end = (t == num_threads - 1) ? items.size() : (t + 1) * chunk_size;
                int local_min = INT_MAX;
                int local_max = 0;
                for (size_t i = start; i < end; ++i) {
                    local_min = std::min(local_min, items[i].get_length());
                    local_max = std::max(local_max, items[i].get_length());
                }
                thread_ranges[t] = {local_min, local_max};
            });
        }

        for (auto& thread : threads) {
            thread.join();
        }

        int min_length = INT_MAX;
        int max_length = 0;
        for (const auto& range : thread_ranges) {
            min_length = std::min(min_length, range.first);
            max_length = std::max(max_length, range.second);
        }

        int range = max_length - min_length + 1;
        if (range > 1'000'000) {
            // Fall back to parallel STL sort
            ParallelSTLSort::sort_by_length(items, ascending);
            return;
        }

        // Parallel counting
        std::vector<std::atomic<size_t>> counts(range);
        for (auto& count : counts) {
            count.store(0);
        }

        threads.clear();
        for (size_t t = 0; t < num_threads; ++t) {
            threads.emplace_back([&, t]() {
                size_t start = t * chunk_size;
                size_t end = (t == num_threads - 1) ? items.size() : (t + 1) * chunk_size;
                for (size_t i = start; i < end; ++i) {
                    int idx = items[i].get_length() - min_length;
                    counts[idx].fetch_add(1);
                }
            });
        }

        for (auto& thread : threads) {
            thread.join();
        }

        // Compute positions
        std::vector<size_t> positions(range);
        positions[0] = 0;
        for (int i = 1; i < range; ++i) {
            positions[i] = positions[i - 1] + counts[i - 1].load();
        }

        // Parallel distribution
        std::vector<item> output(items.size(), item(0, 0, 0, 0.0));
        std::vector<std::atomic<size_t>> atomic_positions(range);
        for (int i = 0; i < range; ++i) {
            atomic_positions[i].store(positions[i]);
        }

        threads.clear();
        for (size_t t = 0; t < num_threads; ++t) {
            threads.emplace_back([&, t]() {
                size_t start = t * chunk_size;
                size_t end = (t == num_threads - 1) ? items.size() : (t + 1) * chunk_size;
                for (size_t i = start; i < end; ++i) {
                    int idx = items[i].get_length() - min_length;
                    size_t pos = atomic_positions[idx].fetch_add(1);
                    output[pos] = items[i];
                }
            });
        }

        for (auto& thread : threads) {
            thread.join();
        }

        if (ascending) {
            items = std::move(output);
        } else {
            // Reverse for descending order
            std::reverse(output.begin(), output.end());
            items = std::move(output);
        }
    }
};

// Lock-free Parallel Radix Sort using moodycamel::ConcurrentQueue
class LockFreeParallelRadixSort {
public:
    static void sort_by_length(std::vector<item>& items, bool ascending = true) {
        if (items.size() < 2) return;

        const size_t num_threads = g_thread_count;
        const size_t min_items_per_thread = 50000;

        // Use serial version for small datasets
        if (items.size() < min_items_per_thread * num_threads) {
            RadixSort::sort_by_length(items, ascending);
            return;
        }

        // Find max length in parallel
        std::atomic<int> global_max{0};
        std::vector<std::thread> threads;
        const size_t chunk_size = items.size() / num_threads;

        for (size_t t = 0; t < num_threads; ++t) {
            threads.emplace_back([&, t]() {
                size_t start = t * chunk_size;
                size_t end = (t == num_threads - 1) ? items.size() : (t + 1) * chunk_size;
                int local_max = 0;

                for (size_t i = start; i < end; ++i) {
                    local_max = std::max(local_max, items[i].get_length());
                }

                // Update global max atomically
                int current = global_max.load();
                while (local_max > current && !global_max.compare_exchange_weak(current, local_max));
            });
        }

        for (auto& thread : threads) {
            thread.join();
        }

        int max_length = global_max.load();

        // Lock-free radix sort
        constexpr int RADIX_BITS = 8;
        constexpr int RADIX_SIZE = 1 << RADIX_BITS;
        constexpr int RADIX_MASK = RADIX_SIZE - 1;

        // Use concurrent queues for each bucket
        std::vector<moodycamel::ConcurrentQueue<item>> buckets(RADIX_SIZE);

        for (int shift = 0; shift < 32 && (max_length >> shift) > 0; shift += RADIX_BITS) {
            threads.clear();

            // Parallel distribution into buckets
            for (size_t t = 0; t < num_threads; ++t) {
                threads.emplace_back([&, t]() {
                    size_t start = t * chunk_size;
                    size_t end = (t == num_threads - 1) ? items.size() : (t + 1) * chunk_size;

                    // Process items in batches to improve cache locality
                    constexpr size_t BATCH_SIZE = 64;
                    std::vector<std::pair<int, item>> batch;
                    batch.reserve(BATCH_SIZE);

                    for (size_t i = start; i < end; ++i) {
                        int bucket_idx = (items[i].get_length() >> shift) & RADIX_MASK;
                        batch.emplace_back(bucket_idx, std::move(items[i]));

                        if (batch.size() == BATCH_SIZE || i == end - 1) {
                            // Enqueue batch
                            for (auto& [idx, it] : batch) {
                                buckets[idx].enqueue(std::move(it));
                            }
                            batch.clear();
                        }
                    }
                });
            }

            for (auto& thread : threads) {
                thread.join();
            }

            // Collect items back from buckets
            items.clear();

            if (ascending) {
                for (int i = 0; i < RADIX_SIZE; ++i) {
                    item temp(0, 0, 0, 0.0); // Dummy item for dequeue
                    while (buckets[i].try_dequeue(temp)) {
                        items.push_back(std::move(temp));
                        temp = item(0, 0, 0, 0.0); // Reset for next dequeue
                    }
                }
            } else {
                for (int i = RADIX_SIZE - 1; i >= 0; --i) {
                    item temp(0, 0, 0, 0.0); // Dummy item for dequeue
                    while (buckets[i].try_dequeue(temp)) {
                        items.push_back(std::move(temp));
                        temp = item(0, 0, 0, 0.0); // Reset for next dequeue
                    }
                }
            }
        }
    }
};

// Lock-free Parallel Counting Sort
class LockFreeParallelCountingSort {
public:
    static void sort_by_length(std::vector<item>& items, bool ascending = true) {
        if (items.size() < 2) return;

        const size_t num_threads = g_thread_count;
        const size_t min_items_per_thread = 50000;

        if (items.size() < min_items_per_thread * num_threads) {
            CountingSort::sort_by_length(items, ascending);
            return;
        }

        // Find min/max in parallel
        struct MinMax {
            int min = INT_MAX;
            int max = 0;
        };

        std::vector<MinMax> thread_results(num_threads);
        std::vector<std::thread> threads;
        const size_t chunk_size = items.size() / num_threads;

        for (size_t t = 0; t < num_threads; ++t) {
            threads.emplace_back([&, t]() {
                size_t start = t * chunk_size;
                size_t end = (t == num_threads - 1) ? items.size() : (t + 1) * chunk_size;

                MinMax local;
                for (size_t i = start; i < end; ++i) {
                    int len = items[i].get_length();
                    local.min = std::min(local.min, len);
                    local.max = std::max(local.max, len);
                }
                thread_results[t] = local;
            });
        }

        for (auto& thread : threads) {
            thread.join();
        }

        int min_length = INT_MAX;
        int max_length = 0;
        for (const auto& result : thread_results) {
            min_length = std::min(min_length, result.min);
            max_length = std::max(max_length, result.max);
        }

        int range = max_length - min_length + 1;
        if (range > 1'000'000) {
            ParallelSTLSort::sort_by_length(items, ascending);
            return;
        }

        // Use concurrent queues for each length value
        std::vector<moodycamel::ConcurrentQueue<item>> length_buckets(range);

        // Parallel distribution
        threads.clear();
        for (size_t t = 0; t < num_threads; ++t) {
            threads.emplace_back([&, t]() {
                size_t start = t * chunk_size;
                size_t end = (t == num_threads - 1) ? items.size() : (t + 1) * chunk_size;

                // Batch processing for better performance
                constexpr size_t BATCH_SIZE = 32;
                std::vector<std::pair<int, item>> batch;
                batch.reserve(BATCH_SIZE);

                for (size_t i = start; i < end; ++i) {
                    int bucket_idx = items[i].get_length() - min_length;
                    batch.emplace_back(bucket_idx, std::move(items[i]));

                    if (batch.size() == BATCH_SIZE || i == end - 1) {
                        for (auto& [idx, it] : batch) {
                            length_buckets[idx].enqueue(std::move(it));
                        }
                        batch.clear();
                    }
                }
            });
        }

        for (auto& thread : threads) {
            thread.join();
        }

        // Parallel collection with bulk dequeue
        items.clear();
        items.reserve(items.size());

        if (ascending) {
            for (int i = 0; i < range; ++i) {
                // Use individual dequeue instead of bulk
                item temp(0, 0, 0, 0.0);
                while (length_buckets[i].try_dequeue(temp)) {
                    items.push_back(std::move(temp));
                    temp = item(0, 0, 0, 0.0); // Reset
                }
            }
        } else {
            for (int i = range - 1; i >= 0; --i) {
                item temp(0, 0, 0, 0.0);
                while (length_buckets[i].try_dequeue(temp)) {
                    items.push_back(std::move(temp));
                    temp = item(0, 0, 0, 0.0); // Reset
                }
            }
        }
    }
};

// SIMD-Optimized RadixSort using AVX2
class SIMDRadixSort {
public:
    static void sort_by_length(std::vector<item>& items, bool ascending = true) {
        if (items.size() < 2) return;

        // For small sizes, fall back to regular radix sort
        if (items.size() < 1000) {
            RadixSort::sort_by_length(items, ascending);
            return;
        }

        constexpr int RADIX_BITS = 8;
        constexpr int RADIX_SIZE = 1 << RADIX_BITS;
        constexpr int RADIX_MASK = RADIX_SIZE - 1;

        // Find max using SIMD
        int max_length = find_max_avx2(items);

        std::vector<item> buffer;
        buffer.reserve(items.size());

        // Aligned arrays for better SIMD performance
        alignas(32) uint32_t count[RADIX_SIZE];
        alignas(32) uint32_t prefix[RADIX_SIZE];

        for (int shift = 0; shift < 32 && (max_length >> shift) > 0; shift += RADIX_BITS) {
            // Clear counts using AVX2
            __m256i zero = _mm256_setzero_si256();
            for (int i = 0; i < RADIX_SIZE; i += 8) {
                _mm256_store_si256(reinterpret_cast<__m256i*>(&count[i]), zero);
            }

            // Count occurrences - process multiple items at once where possible
            size_t i = 0;

            // Process 8 items at a time for better cache usage
            for (; i + 8 <= items.size(); i += 8) {
                // Prefetch next batch
                _mm_prefetch(reinterpret_cast<const char*>(&items[i + 8]), _MM_HINT_T0);

                // Extract lengths and compute buckets
                alignas(32) uint32_t buckets[8];
                for (int j = 0; j < 8; ++j) {
                    buckets[j] = (items[i + j].get_length() >> shift) & RADIX_MASK;
                    count[buckets[j]]++;
                }
            }

            // Handle remaining items
            for (; i < items.size(); ++i) {
                int bucket = (items[i].get_length() >> shift) & RADIX_MASK;
                count[bucket]++;
            }

            // Compute prefix sum
            prefix[0] = 0;
            if (ascending) {
                // Vectorized prefix sum for better performance
                for (int j = 1; j < RADIX_SIZE; ++j) {
                    prefix[j] = prefix[j - 1] + count[j - 1];
                }
            } else {
                prefix[RADIX_SIZE - 1] = 0;
                for (int j = RADIX_SIZE - 2; j >= 0; --j) {
                    prefix[j] = prefix[j + 1] + count[j + 1];
                }
            }

            // Distribution phase
            buffer.clear();
            buffer.resize(items.size(), item(0, 0, 0, 0.0));

            // Reset prefix for distribution
            std::memcpy(count, prefix, sizeof(prefix));

            // Distribute with prefetching
            for (size_t j = 0; j < items.size(); ++j) {
                // Prefetch next items
                if (j + 4 < items.size()) {
                    _mm_prefetch(reinterpret_cast<const char*>(&items[j + 4]), _MM_HINT_T0);
                }

                int bucket = (items[j].get_length() >> shift) & RADIX_MASK;
                buffer[count[bucket]++] = std::move(items[j]);
            }

            items.swap(buffer);
        }
    }

private:
    static int find_max_avx2(const std::vector<item>& items) {
        __m256i max_vec = _mm256_setzero_si256();

        size_t i = 0;
        // Process 8 elements at a time
        for (; i + 8 <= items.size(); i += 8) {
            alignas(32) int32_t lengths[8];
            for (int j = 0; j < 8; ++j) {
                lengths[j] = items[i + j].get_length();
            }

            __m256i curr = _mm256_load_si256(reinterpret_cast<const __m256i*>(lengths));
            max_vec = _mm256_max_epi32(max_vec, curr);
        }

        // Find maximum in the vector
        alignas(32) int32_t max_arr[8];
        _mm256_store_si256(reinterpret_cast<__m256i*>(max_arr), max_vec);

        int max_val = *std::max_element(max_arr, max_arr + 8);

        // Handle remaining elements
        for (; i < items.size(); ++i) {
            max_val = std::max(max_val, items[i].get_length());
        }

        return max_val;
    }
};

// Optimized Three-Way Radix Quicksort
class RadixQuickSort {
public:
    static void sort_by_length(std::vector<item>& items, bool ascending = true) {
        if (items.size() < 2) return;

        // Extract lengths for faster access
        std::vector<std::pair<int, size_t>> length_index;
        length_index.reserve(items.size());

        for (size_t i = 0; i < items.size(); ++i) {
            length_index.emplace_back(items[i].get_length(), i);
        }

        // Sort using three-way radix quicksort
        radix_quicksort(length_index, 0, length_index.size(),
                        find_max_bit(length_index), ascending);

        // Reorder items based on sorted indices
        std::vector<item> sorted_items;
        sorted_items.reserve(items.size());

        for (const auto& [length, idx] : length_index) {
            sorted_items.push_back(std::move(items[idx]));
        }

        items = std::move(sorted_items);
    }

private:
    static int find_max_bit(const std::vector<std::pair<int, size_t>>& data) {
        int max_val = 0;
        for (const auto& [val, idx] : data) {
            max_val = std::max(max_val, val);
        }
        return max_val == 0 ? 0 : 31 - __builtin_clz(max_val);
    }

    static void radix_quicksort(std::vector<std::pair<int, size_t>>& data,
                                size_t start, size_t end, int bit, bool ascending) {
        if (end - start <= 1 || bit < 0) return;

        // Three-way partition based on bit
        size_t lt = start, gt = end, i = start;

        while (i < gt) {
            int bit_val = (data[i].first >> bit) & 1;

            if (bit_val == 0) {
                std::swap(data[lt++], data[i++]);
            } else {
                std::swap(data[i], data[--gt]);
            }
        }

        // Recursive calls
        if (!ascending) {
            radix_quicksort(data, gt, end, bit - 1, ascending);
            radix_quicksort(data, start, lt, bit - 1, ascending);
        } else {
            radix_quicksort(data, start, lt, bit - 1, ascending);
            radix_quicksort(data, gt, end, bit - 1, ascending);
        }
    }
};

// Hybrid Intro-Radix Sort
class IntroRadixSort {
public:
    static void sort_by_length(std::vector<item>& items, bool ascending = true) {
        if (items.size() < 2) return;

        // Use introsort-like approach: start with quicksort, fall back to radix
        intro_radix_sort(items, 0, items.size(), 2 * std::log2(items.size()), ascending);
    }

private:
    static constexpr size_t INSERTION_THRESHOLD = 32;
    static constexpr size_t RADIX_THRESHOLD = 1000;

    static void intro_radix_sort(std::vector<item>& items, size_t start, size_t end,
                                 int depth_limit, bool ascending) {
        size_t size = end - start;

        if (size < 2) return;

        // Small arrays: use insertion sort
        if (size <= INSERTION_THRESHOLD) {
            insertion_sort(items, start, end, ascending);
            return;
        }

        // Depth limit exceeded or large array: use radix sort
        if (depth_limit == 0 || size > RADIX_THRESHOLD) {
            // Extract subrange and sort with radix
            std::vector<item> sub_items(
                std::make_move_iterator(items.begin() + start),
                std::make_move_iterator(items.begin() + end)
                );

            RadixSort::sort_by_length(sub_items, ascending);

            // Move back
            std::move(sub_items.begin(), sub_items.end(), items.begin() + start);
            return;
        }

        // Otherwise, use quicksort with median-of-three pivot
        size_t pivot_idx = partition_median_of_three(items, start, end, ascending);

        intro_radix_sort(items, start, pivot_idx, depth_limit - 1, ascending);
        intro_radix_sort(items, pivot_idx + 1, end, depth_limit - 1, ascending);
    }

    static void insertion_sort(std::vector<item>& items, size_t start, size_t end, bool ascending) {
        for (size_t i = start + 1; i < end; ++i) {
            item key = std::move(items[i]);
            size_t j = i;

            if (ascending) {
                while (j > start && items[j - 1].get_length() > key.get_length()) {
                    items[j] = std::move(items[j - 1]);
                    --j;
                }
            } else {
                while (j > start && items[j - 1].get_length() < key.get_length()) {
                    items[j] = std::move(items[j - 1]);
                    --j;
                }
            }

            items[j] = std::move(key);
        }
    }

    static size_t partition_median_of_three(std::vector<item>& items, size_t start,
                                            size_t end, bool ascending) {
        size_t mid = start + (end - start) / 2;
        size_t last = end - 1;

        // Sort first, middle, last
        if (ascending) {
            if (items[mid] < items[start]) std::swap(items[start], items[mid]);
            if (items[last] < items[start]) std::swap(items[start], items[last]);
            if (items[last] < items[mid]) std::swap(items[mid], items[last]);
        } else {
            if (items[mid] > items[start]) std::swap(items[start], items[mid]);
            if (items[last] > items[start]) std::swap(items[start], items[last]);
            if (items[last] > items[mid]) std::swap(items[mid], items[last]);
        }

        // Place pivot at end-1
        std::swap(items[mid], items[last - 1]);
        int pivot_length = items[last - 1].get_length();

        // Partition
        size_t i = start;
        size_t j = last - 1;

        while (true) {
            if (ascending) {
                while (items[++i].get_length() < pivot_length);
                while (items[--j].get_length() > pivot_length);
            } else {
                while (items[++i].get_length() > pivot_length);
                while (items[--j].get_length() < pivot_length);
            }

            if (i >= j) break;
            std::swap(items[i], items[j]);
        }

        std::swap(items[i], items[last - 1]);
        return i;
    }
};

class SIMDRadixSortV2 {
public:
    static void sort_by_length(std::vector<item>& items, bool ascending = true) {
        if (items.size() < 2) return;

        // Use different strategies based on size
        if (items.size() < 64) {
            // For very small arrays, use insertion sort
            insertion_sort(items, ascending);
            return;
        } else if (items.size() < 1000) {
            // For small arrays, use regular radix sort (less overhead)
            RadixSort::sort_by_length(items, ascending);
            return;
        }

        constexpr int RADIX_BITS = 8;
        constexpr int RADIX_SIZE = 1 << RADIX_BITS;
        constexpr int RADIX_MASK = RADIX_SIZE - 1;

        // Find max using SIMD with unrolling
        int max_length = find_max_avx2_unrolled(items);

        // Pre-allocate buffer to avoid repeated allocations
        std::vector<item> buffer(items.size(), item(0, 0, 0, 0.0));

        // Use stack allocation for small arrays
        alignas(64) uint32_t count[RADIX_SIZE];  // 64-byte alignment for cache lines
        alignas(64) uint32_t prefix[RADIX_SIZE];

        // Calculate number of passes needed
        int num_passes = 0;
        for (int temp = max_length; temp > 0; temp >>= RADIX_BITS) {
            num_passes++;
        }

        for (int pass = 0; pass < num_passes; pass++) {
            int shift = pass * RADIX_BITS;

            // Clear counts using AVX2 - unrolled for better performance
            __m256i zero = _mm256_setzero_si256();
            for (int i = 0; i < RADIX_SIZE; i += 32) {
                _mm256_store_si256(reinterpret_cast<__m256i*>(&count[i]), zero);
                _mm256_store_si256(reinterpret_cast<__m256i*>(&count[i + 8]), zero);
                _mm256_store_si256(reinterpret_cast<__m256i*>(&count[i + 16]), zero);
                _mm256_store_si256(reinterpret_cast<__m256i*>(&count[i + 24]), zero);
            }

            // Counting phase with software prefetching and unrolling
            size_t i = 0;

            // Process 16 items at a time with prefetching
            for (; i + 16 <= items.size(); i += 16) {
                // Prefetch next batch
                _mm_prefetch(reinterpret_cast<const char*>(&items[i + 16]), _MM_HINT_T0);
                _mm_prefetch(reinterpret_cast<const char*>(&items[i + 24]), _MM_HINT_T0);

// Process 16 items with manual unrolling
#pragma unroll(16)
                for (int j = 0; j < 16; ++j) {
                    uint32_t bucket = (items[i + j].get_length() >> shift) & RADIX_MASK;
                    count[bucket]++;
                }
            }

            // Handle remaining items
            for (; i < items.size(); ++i) {
                uint32_t bucket = (items[i].get_length() >> shift) & RADIX_MASK;
                count[bucket]++;
            }

            // Compute prefix sum with cache-friendly access
            if (ascending) {
                prefix[0] = 0;
                // Use SIMD for prefix sum where possible
                uint32_t running_sum = 0;
                for (int j = 0; j < RADIX_SIZE - 1; ++j) {
                    prefix[j + 1] = running_sum + count[j];
                    running_sum = prefix[j + 1];
                }
            } else {
                prefix[RADIX_SIZE - 1] = 0;
                uint32_t running_sum = 0;
                for (int j = RADIX_SIZE - 2; j >= 0; --j) {
                    prefix[j] = running_sum + count[j + 1];
                    running_sum = prefix[j];
                }
            }

            // Distribution phase with write combining
            for (size_t j = 0; j < items.size(); ++j) {
                // Use non-temporal stores for large datasets to bypass cache
                if (items.size() > 1000000 && j % 64 == 0) {
                    _mm_prefetch(reinterpret_cast<const char*>(&items[j + 64]), _MM_HINT_NTA);
                }

                uint32_t bucket = (items[j].get_length() >> shift) & RADIX_MASK;
                buffer[prefix[bucket]++] = std::move(items[j]);
            }

            items.swap(buffer);
        }
    }

private:
    static void insertion_sort(std::vector<item>& items, bool ascending) {
        for (size_t i = 1; i < items.size(); ++i) {
            item key = std::move(items[i]);
            size_t j = i;

            if (ascending) {
                while (j > 0 && items[j - 1].get_length() > key.get_length()) {
                    items[j] = std::move(items[j - 1]);
                    --j;
                }
            } else {
                while (j > 0 && items[j - 1].get_length() < key.get_length()) {
                    items[j] = std::move(items[j - 1]);
                    --j;
                }
            }

            items[j] = std::move(key);
        }
    }

    static int find_max_avx2_unrolled(const std::vector<item>& items) {
        __m256i max_vec1 = _mm256_setzero_si256();
        __m256i max_vec2 = _mm256_setzero_si256();

        size_t i = 0;
        // Process 16 elements at a time (2x8)
        for (; i + 16 <= items.size(); i += 16) {
            alignas(32) int32_t lengths1[8];
            alignas(32) int32_t lengths2[8];

// Load lengths
#pragma unroll(8)
            for (int j = 0; j < 8; ++j) {
                lengths1[j] = items[i + j].get_length();
                lengths2[j] = items[i + j + 8].get_length();
            }

            __m256i curr1 = _mm256_load_si256(reinterpret_cast<const __m256i*>(lengths1));
            __m256i curr2 = _mm256_load_si256(reinterpret_cast<const __m256i*>(lengths2));

            max_vec1 = _mm256_max_epi32(max_vec1, curr1);
            max_vec2 = _mm256_max_epi32(max_vec2, curr2);
        }

        // Combine the two max vectors
        __m256i max_vec = _mm256_max_epi32(max_vec1, max_vec2);

        // Find maximum in the vector using horizontal max
        __m128i max_128 = _mm_max_epi32(
            _mm256_castsi256_si128(max_vec),
            _mm256_extracti128_si256(max_vec, 1)
            );

        max_128 = _mm_max_epi32(max_128, _mm_shuffle_epi32(max_128, _MM_SHUFFLE(1, 0, 3, 2)));
        max_128 = _mm_max_epi32(max_128, _mm_shuffle_epi32(max_128, _MM_SHUFFLE(2, 3, 0, 1)));

        int max_val = _mm_cvtsi128_si32(max_128);

        // Handle remaining elements
        for (; i < items.size(); ++i) {
            max_val = std::max(max_val, items[i].get_length());
        }

        return max_val;
    }
};

}
