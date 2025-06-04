#pragma once

#include "pack_strategy.h"
#include <thread>
#include <future>
#include <mutex>
#include <atomic>
#include <algorithm>

/**
 * @brief Parallel pack strategy using multiple threads
 * Divides items into chunks and processes them in parallel
 */
class parallel_pack_strategy : public pack_strategy {
private:
    unsigned int m_num_threads;

    /**
     * @brief Worker function for a thread to process a chunk of items
     * @param items Items to process
     * @param start_idx Starting index in the items vector
     * @param end_idx Ending index in the items vector
     * @param max_items Maximum items per pack
     * @param max_weight Maximum weight per pack
     * @param result_packs Vector to store resulting packs
     * @param next_pack_number Atomic counter for pack numbers
     * @param mutex Mutex for thread synchronization
     */
    void worker_thread(
        const std::vector<item>& items,
        size_t start_idx,
        size_t end_idx,
        int max_items,
        double max_weight,
        std::vector<pack>& result_packs,
        std::atomic<int>& next_pack_number,
        std::mutex& mutex) {

        // Process items in this thread's chunk
        std::vector<pack> local_packs;
        local_packs.reserve(std::max<size_t>(16, static_cast<size_t>((end_idx - start_idx) * 0.00222) + 8));

        // Get first pack number for this thread
        int pack_number = next_pack_number.fetch_add(1);
        local_packs.emplace_back(pack_number);

        for (size_t i = start_idx; i < end_idx; ++i) {
            const auto& item = items[i];
            int remaining_quantity = item.get_quantity();

            while (remaining_quantity > 0) {
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
                    // Get next pack number atomically
                    pack_number = next_pack_number.fetch_add(1);
                    local_packs.emplace_back(pack_number);
                }
            }
        }

        // Merge local results into the shared result vector
        {
            std::lock_guard<std::mutex> lock(mutex);
            result_packs.insert(result_packs.end(), local_packs.begin(), local_packs.end());
        }
    }

public:
    /**
     * @brief Construct a new parallel packing strategy
     * @param num_threads Number of threads to use (0 = use hardware concurrency)
     */
    explicit parallel_pack_strategy(int thread_count = 4)
        : m_num_threads(thread_count)
    {
        if (m_num_threads == 0) {
            m_num_threads = std::thread::hardware_concurrency();
        }
    }

    /**
     * @brief Pack items into packs using multiple threads
     * @param items Items to pack
     * @param max_items Maximum items per pack
     * @param max_weight Maximum weight per pack
     * @return std::vector<pack> Vector of packs
     */
    std::vector<pack> pack_items(const std::vector<item>& items,
                               int max_items,
                               double max_weight) override {
        // If items are few or we have only 1 thread, use sequential approach
        // Hybrid approach
        if (items.size() < 5000 || m_num_threads == 1) {
            std::vector<pack> packs;
            packs.reserve(std::max<size_t>(64, static_cast<size_t>(items.size() * 0.00222) + 16));
            int pack_number = 1;
            packs.emplace_back(pack_number);

            for (const auto& i : items) {
                int remaining_quantity = i.get_quantity();

                while (remaining_quantity > 0) {
                    pack& current_pack = packs.back();
                    int added_quantity =
                        current_pack.add_partial_item(i.get_id(), i.get_length(), remaining_quantity,
                                                      i.get_weight(), max_items, max_weight);

                    if (added_quantity > 0) {
                        remaining_quantity -= added_quantity;
                    } else {
                        packs.emplace_back(++pack_number);
                    }
                }
            }
            return packs;
        }

        // For parallel processing
        std::vector<pack> result_packs;
        std::mutex result_mutex;
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

            threads.emplace_back(&parallel_pack_strategy::worker_thread,
                                 this,
                                 std::ref(items),
                                 start_idx,
                                 end_idx,
                                 max_items,
                                 max_weight,
                                 std::ref(result_packs),
                                 std::ref(next_pack_number),
                                 std::ref(result_mutex));

            start_idx = end_idx;
        }

        // Wait for all threads to complete
        for (auto& thread : threads) {
            thread.join();
        }

        return result_packs;
    }

    std::string get_name() const override {
        return "Parallel(" + std::to_string(m_num_threads) + " threads)";
    }
};
