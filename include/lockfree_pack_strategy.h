#pragma once

#include "pack_strategy.h"
#include <concurrentqueue/moodycamel/concurrentqueue.h>
#include <thread>
#include <future>
#include <atomic>
#include <algorithm>

/**
 * @brief Lock-free parallel pack strategy using moodycamel::ConcurrentQueue
 * Same algorithm as parallel_pack_strategy but with lock-free result collection
 */
class lockfree_pack_strategy : public pack_strategy {
private:
    unsigned int m_num_threads;

    /**
     * @brief Worker function for a thread to process a chunk of items
     */
    void worker_thread(
        const std::vector<item>& items,
        size_t start_idx,
        size_t end_idx,
        int max_items,
        double max_weight,
        moodycamel::ConcurrentQueue<pack>& result_queue,
        std::atomic<int>& next_pack_number) {

        // SAFETY: Validate constraints to prevent infinite loops
        max_items = std::max(1, max_items);
        max_weight = std::max(0.1, max_weight);

        // Process items in this thread's chunk
        std::vector<pack> local_packs;
        // SAFETY: Limit initial allocation to prevent OOM with extreme values
        const size_t max_safe_reserve = std::min<size_t>(20000, (end_idx - start_idx) / 10 + 500);
        local_packs.reserve(std::min(max_safe_reserve,
                                     std::max<size_t>(16, static_cast<size_t>((end_idx - start_idx) * 0.00222) + 8)));

        // Get first pack number for this thread
        int pack_number = next_pack_number.fetch_add(1);
        local_packs.emplace_back(pack_number);

        // SAFETY: Add a safety counter to prevent infinite loops
        const int max_iterations = 500000; // Reasonable upper limit per thread
        int safety_counter = 0;

        for (size_t i = start_idx; i < end_idx; ++i) {
            const auto& item = items[i];
            // SAFETY: Skip items with non-positive quantities
            if (item.get_quantity() <= 0) continue;

            int remaining_quantity = item.get_quantity();

            while (remaining_quantity > 0) {
                // SAFETY: Check for potential infinite loop
                if (++safety_counter > max_iterations) {
                    // Force exit the loop if we've exceeded reasonable iterations
                    break;
                }

                pack& current_pack = local_packs.back();
                int added_quantity = current_pack.add_partial_item(
                    item.get_id(),
                    item.get_length(),
                    remaining_quantity,
                    item.get_weight(),
                    max_items,
                    max_weight);

                if (added_quantity > 0) {
                    remaining_quantity -= added_quantity;
                } else {
                    // Check if this item can never fit (weight exceeds max_weight)
                    if (item.get_weight() > max_weight) {
                        // Item is too heavy to fit in any pack, skip it
                        remaining_quantity = 0;
                        break;
                    }
                    // Fallback: If pack is empty but item should fit, something else is wrong
                    if (current_pack.is_empty()) {
                        remaining_quantity = 0;
                        break;
                    }

                    // SAFETY: Limit maximum number of packs to prevent OOM
                    if (local_packs.size() >= max_safe_reserve) {
                        // Force exit if we've created too many packs
                        remaining_quantity = 0;
                        break;
                    }
                    // Get next pack number atomically
                    pack_number = next_pack_number.fetch_add(1);
                    local_packs.emplace_back(pack_number);
                }
            }
        }

        // Enqueue local results into the lock-free queue
        for (auto& p : local_packs) {
            if (!p.is_empty()) {
                result_queue.enqueue(std::move(p));
            }
        }
    }

public:
    /**
     * @brief Construct a new lock-free parallel packing strategy
     * @param num_threads Number of threads to use (0 = use hardware concurrency)
     */
    explicit lockfree_pack_strategy(int thread_count = 4)
        : m_num_threads(thread_count)
    {
        if (m_num_threads == 0) {
            m_num_threads = std::thread::hardware_concurrency();
        }
    }

    /**
     * @brief Pack items into packs using multiple threads with lock-free queue
     */
    std::vector<pack> pack_items(const std::vector<item>& items,
                                 int max_items,
                                 double max_weight) override {
        // SAFETY: Validate constraints to prevent infinite loops
        max_items = std::max(1, max_items);
        max_weight = std::max(0.1, max_weight);

        // SAFETY: Limit thread count to a reasonable number
        m_num_threads = std::min(static_cast<unsigned int>(32),
                                 std::max(static_cast<unsigned int>(1), m_num_threads));

        // If items are few or we have only 1 thread, use sequential approach
        // Hybrid approach
        if (items.size() < 5000 || m_num_threads == 1) {
            // SAFETY: Same fixes as in blocking strategy
            std::vector<pack> packs;
            const size_t max_safe_reserve = std::min<size_t>(100000, items.size() / 10 + 1000);
            packs.reserve(std::min(max_safe_reserve,
                                   std::max<size_t>(64, static_cast<size_t>(items.size() * 0.00222) + 16)));
            int pack_number = 1;
            packs.emplace_back(pack_number);

            // SAFETY: Add a safety counter to prevent infinite loops
            const int max_iterations = 1000000; // Reasonable upper limit
            int safety_counter = 0;

            for (const auto& i : items) {
                // SAFETY: Skip items with non-positive quantities
                if (i.get_quantity() <= 0) continue;

                int remaining_quantity = i.get_quantity();

                while (remaining_quantity > 0) {
                    // SAFETY: Check for potential infinite loop
                    if (++safety_counter > max_iterations) {
                        // Force exit the loop if we've exceeded reasonable iterations
                        break;
                    }

                    pack& current_pack = packs.back();
                    int added_quantity =
                        current_pack.add_partial_item(i.get_id(), i.get_length(), remaining_quantity,
                                                      i.get_weight(), max_items, max_weight);

                    if (added_quantity > 0) {
                        remaining_quantity -= added_quantity;
                    } else {
                        // Check if this item can never fit (weight exceeds max_weight)
                        if (i.get_weight() > max_weight) {
                            // Item is too heavy to fit in any pack, skip it
                            remaining_quantity = 0;
                            break;
                        }
                        // Fallback: If pack is empty but item should fit, something else is wrong
                        if (current_pack.is_empty()) {
                            remaining_quantity = 0;
                            break;
                        }

                        // SAFETY: Limit maximum number of packs to prevent OOM
                        if (packs.size() >= max_safe_reserve) {
                            // Force exit if we've created too many packs
                            remaining_quantity = 0;
                            break;
                        }
                        packs.emplace_back(++pack_number);
                    }
                }
            }
            return packs;
        }

        // For parallel processing with lock-free queue
        moodycamel::ConcurrentQueue<pack> result_queue;
        std::atomic<int> next_pack_number{1};

        // Calculate chunk size for each thread
        size_t chunk_size = items.size() / m_num_threads;
        size_t remainder = items.size() % m_num_threads;

        // Create and start threads
        std::vector<std::thread> threads;
        threads.reserve(m_num_threads);

        size_t start_idx = 0;
        for (unsigned int i = 0; i < m_num_threads; ++i) {
            // Distribute remainder items among first 'remainder' threads
            size_t thread_chunk_size = chunk_size + (i < remainder ? 1 : 0);
            size_t end_idx = start_idx + thread_chunk_size;

            threads.emplace_back(&lockfree_pack_strategy::worker_thread,
                                 this,
                                 std::ref(items),
                                 start_idx,
                                 end_idx,
                                 max_items,
                                 max_weight,
                                 std::ref(result_queue),
                                 std::ref(next_pack_number));

            start_idx = end_idx;
        }

        // Wait for all threads to complete
        for (auto& thread : threads) {
            thread.join();
        }

        // Collect results from lock-free queue
        std::vector<pack> result_packs;
        pack p(0);

        // Drain the queue
        while (result_queue.try_dequeue(p)) {
            result_packs.push_back(std::move(p));
        }

        return result_packs;
    }

    std::string get_name() const override {
        return "Lock-free(" + std::to_string(m_num_threads) + " threads)";
    }
};
