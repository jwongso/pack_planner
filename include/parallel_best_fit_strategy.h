#pragma once

#include "pack_strategy.h"
#include <thread>
#include <future>
#include <mutex>
#include <atomic>
#include <algorithm>
#include <queue>
#include <unordered_map>

/**
 * @brief Parallel Best Fit pack strategy using multiple threads
 * Each thread maintains its own set of packs and uses best fit algorithm
 * Results are merged at the end with global pack numbering
 */
class parallel_best_fit_pack_strategy : public pack_strategy {
private:
    unsigned int m_num_threads;  // Fixed: Use consistent unsigned int
    static constexpr double WEIGHT_EPSILON = 1e-3;  // Configurable epsilon
    static constexpr size_t WORK_BATCH_SIZE = 100;   // Items per work batch

    /**
     * @brief Structure to track pack capacity for priority queue
     */
    struct pack_capacity {
        int pack_index;
        double remaining_weight;
        int remaining_items;

        pack_capacity(int idx, double weight, int items)
            : pack_index(idx), remaining_weight(weight), remaining_items(items) {}
    };

    /**
     * @brief Comparator for priority queue - min-heap based on remaining weight
     */
    struct capacity_comparator {
        bool operator()(const pack_capacity& a, const pack_capacity& b) const {
            if (std::abs(a.remaining_weight - b.remaining_weight) < WEIGHT_EPSILON) {  // Use consistent epsilon
                return a.remaining_items > b.remaining_items;
            }
            return a.remaining_weight > b.remaining_weight;
        }
    };

    /**
     * @brief Worker function for a thread to process items using work stealing
     */
    void worker_thread_work_stealing(
        const std::vector<item>& items,
        std::atomic<size_t>& next_item_idx,
        int max_items,
        double max_weight,
        std::vector<pack>& result_packs,
        std::atomic<int>& next_pack_number,
        std::mutex& mutex) {

        // Validate constraints
        max_items = std::max(1, max_items);
        max_weight = std::max(0.1, max_weight);

        // Local data structures
        std::vector<pack> local_packs;
        std::priority_queue<pack_capacity, std::vector<pack_capacity>, capacity_comparator> pack_queue;

        const size_t max_safe_reserve = 5000;
        local_packs.reserve(max_safe_reserve / 4);

        const int max_iterations = 500000;
        int safety_counter = 0;

        // Work stealing loop
        while (true) {
            size_t start_idx = next_item_idx.fetch_add(WORK_BATCH_SIZE);
            if (start_idx >= items.size()) break;

            size_t end_idx = std::min(start_idx + WORK_BATCH_SIZE, items.size());

            // Process this batch of items
            for (size_t i = start_idx; i < end_idx; ++i) {
                const auto& item = items[i];

                // Early validation for oversized items
                if (item.get_weight() > max_weight) {
                    continue; // Skip oversized items
                }

                if (item.get_quantity() <= 0) continue;

                int remaining_quantity = item.get_quantity();

                while (remaining_quantity > 0) {
                    if (++safety_counter > max_iterations) break;

                    int added_quantity = 0;
                    std::vector<pack_capacity> temp_storage;

                    // Best fit search in local packs
                    while (!pack_queue.empty()) {
                        pack_capacity current = pack_queue.top();
                        pack_queue.pop();

                        if (current.remaining_items > 0 &&
                            current.remaining_weight >= item.get_weight()) {

                            pack& target_pack = local_packs[current.pack_index];
                            added_quantity = target_pack.add_partial_item(
                                item.get_id(), item.get_length(), remaining_quantity,
                                item.get_weight(), max_items, max_weight);

                            if (added_quantity > 0) {
                                double new_remaining_weight = target_pack.get_remaining_weight_capacity(max_weight);
                                int new_remaining_items = target_pack.get_remaining_item_capacity(max_items);

                                if (new_remaining_items > 0 && new_remaining_weight > WEIGHT_EPSILON) {
                                    temp_storage.emplace_back(current.pack_index, new_remaining_weight, new_remaining_items);
                                }
                                break;
                            }
                        }
                        temp_storage.push_back(current);
                    }

                    // Restore queue
                    for (const auto& pack_cap : temp_storage) {
                        pack_queue.push(pack_cap);
                    }

                    if (added_quantity > 0) {
                        remaining_quantity -= added_quantity;
                    } else {
                        // Create new pack
                        if (local_packs.size() >= max_safe_reserve) {
                            remaining_quantity = 0;
                            break;
                        }

                        int temp_pack_number = next_pack_number.fetch_add(1);
                        local_packs.emplace_back(temp_pack_number);
                        pack& new_pack = local_packs.back();

                        added_quantity = new_pack.add_partial_item(
                            item.get_id(), item.get_length(), remaining_quantity,
                            item.get_weight(), max_items, max_weight);

                        if (added_quantity > 0) {
                            remaining_quantity -= added_quantity;

                            double remaining_weight = new_pack.get_remaining_weight_capacity(max_weight);
                            int remaining_items = new_pack.get_remaining_item_capacity(max_items);

                            if (remaining_items > 0 && remaining_weight > WEIGHT_EPSILON) {
                                pack_queue.emplace(local_packs.size() - 1, remaining_weight, remaining_items);
                            }
                        } else {
                            remaining_quantity = 0;
                        }
                    }
                }
            }
        }

        // Merge results
        {
            std::lock_guard<std::mutex> lock(mutex);
            const size_t max_total_packs = 20000;
            if (result_packs.size() < max_total_packs) {
                result_packs.insert(result_packs.end(),
                                local_packs.begin(),
                                local_packs.begin() + std::min(local_packs.size(),
                                                                max_total_packs - result_packs.size()));
            }
        }
    }

public:
    /**
     * @brief Construct a new parallel best fit packing strategy
     * @param thread_count Number of threads to use (0 = use hardware concurrency)
     */
    explicit parallel_best_fit_pack_strategy(int thread_count = 4)
        : m_num_threads(static_cast<unsigned int>(thread_count))
    {
        if (m_num_threads == 0) {
            m_num_threads = std::thread::hardware_concurrency();
        }
    }

    /**
     * @brief Pack items into packs using multiple threads with best fit algorithm
     */
    std::vector<pack> pack_items(const std::vector<item>& items,
                            int max_items,
                            double max_weight) override {
        // SAFETY: Validate constraints
        max_items = std::max(1, max_items);
        max_weight = std::max(0.1, max_weight);

        // SAFETY: Limit thread count
        m_num_threads = std::min(32u, std::max(1u, m_num_threads));

        // Use parallel processing for large datasets
        if (items.size() >= 5000 && m_num_threads > 1) {
            std::vector<pack> result_packs;
            std::mutex result_mutex;
            std::atomic<int> next_pack_number{1};
            std::atomic<size_t> next_item_idx{0};

            // Create threads with work stealing
            std::vector<std::thread> threads;
            threads.reserve(m_num_threads);

            for (unsigned int i = 0; i < m_num_threads; ++i) {
                threads.emplace_back(&parallel_best_fit_pack_strategy::worker_thread_work_stealing,
                                    this,
                                    std::ref(items),
                                    std::ref(next_item_idx),
                                    max_items,
                                    max_weight,
                                    std::ref(result_packs),
                                    std::ref(next_pack_number),
                                    std::ref(result_mutex));
            }

            // Wait for completion
            for (auto& thread : threads) {
                thread.join();
            }

            // Renumber packs sequentially
            for (size_t i = 0; i < result_packs.size(); ++i) {
                result_packs[i].set_pack_number(static_cast<int>(i + 1));
            }

            return result_packs;
        }
        else {
            // Sequential fallback for small datasets or single thread
            std::vector<pack> packs;
            std::priority_queue<pack_capacity, std::vector<pack_capacity>, capacity_comparator> pack_queue;

            const size_t max_safe_reserve = 10000;
            packs.reserve(std::min(max_safe_reserve,
                        std::max<size_t>(64, static_cast<size_t>(items.size() * 0.002) + 16)));

            int pack_number = 1;
            const int max_iterations = 1000000;
            int safety_counter = 0;

            for (const auto& item : items) {
                // Add validation for oversized items in sequential path too
                if (item.get_weight() > max_weight) {
                    continue; // Skip oversized items
                }

                if (item.get_quantity() <= 0) continue;

                int remaining_quantity = item.get_quantity();

                while (remaining_quantity > 0) {
                    if (++safety_counter > max_iterations) {
                        break;
                    }

                    int added_quantity = 0;
                    std::vector<pack_capacity> temp_storage;

                    while (!pack_queue.empty()) {
                        pack_capacity current = pack_queue.top();
                        pack_queue.pop();

                        if (current.remaining_items > 0 &&
                            current.remaining_weight >= item.get_weight()) {

                            pack& target_pack = packs[current.pack_index];
                            added_quantity = target_pack.add_partial_item(
                                item.get_id(), item.get_length(), remaining_quantity,
                                item.get_weight(), max_items, max_weight);

                            if (added_quantity > 0) {
                                double new_remaining_weight = target_pack.get_remaining_weight_capacity(max_weight);
                                int new_remaining_items = target_pack.get_remaining_item_capacity(max_items);

                                // Fixed: Use consistent WEIGHT_EPSILON
                                if (new_remaining_items > 0 && new_remaining_weight > WEIGHT_EPSILON) {
                                    temp_storage.emplace_back(current.pack_index, new_remaining_weight, new_remaining_items);
                                }
                                break;
                            }
                        }

                        temp_storage.push_back(current);
                    }

                    for (const auto& pack_cap : temp_storage) {
                        pack_queue.push(pack_cap);
                    }

                    if (added_quantity > 0) {
                        remaining_quantity -= added_quantity;
                    } else {
                        if (packs.size() >= max_safe_reserve) {
                            remaining_quantity = 0;
                            break;
                        }

                        packs.emplace_back(pack_number++);
                        pack& new_pack = packs.back();

                        added_quantity = new_pack.add_partial_item(
                            item.get_id(), item.get_length(), remaining_quantity,
                            item.get_weight(), max_items, max_weight);

                        if (added_quantity > 0) {
                            remaining_quantity -= added_quantity;

                            double remaining_weight = new_pack.get_remaining_weight_capacity(max_weight);
                            int remaining_items = new_pack.get_remaining_item_capacity(max_items);

                            // Fixed: Use consistent WEIGHT_EPSILON
                            if (remaining_items > 0 && remaining_weight > WEIGHT_EPSILON) {
                                pack_queue.emplace(packs.size() - 1, remaining_weight, remaining_items);
                            }
                        } else {
                            remaining_quantity = 0;
                        }
                    }
                }
            }
            return packs;
        }
    }

    std::string get_name() const override {
        return "Parallel Best Fit(" + std::to_string(m_num_threads) + " threads)";
    }
};
