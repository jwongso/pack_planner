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
    int m_thread_count;

    /**
     * @brief Pack a chunk of items
     * @param items_chunk Chunk of items to pack
     * @param max_items Maximum items per pack
     * @param max_weight Maximum weight per pack
     * @param start_pack_number Starting pack number for this chunk
     * @return std::vector<pack> Packs created from this chunk
     */
    std::vector<pack> pack_chunk(const std::vector<item>& items_chunk,
                               int max_items,
                               double max_weight,
                               int start_pack_number) {
        std::vector<pack> packs;
        packs.reserve(std::max<size_t>(16, items_chunk.size() / 10));
        int pack_number = start_pack_number;
        packs.emplace_back(pack_number);

        for (const auto& item : items_chunk) {
            int remaining_quantity = item.get_quantity();

            while (remaining_quantity > 0) {
                pack& current_pack = packs.back();
                int added_quantity = current_pack.add_partial_item(
                    item.get_id(), item.get_length(), remaining_quantity,
                    item.get_weight(), max_items, max_weight);

                if (added_quantity > 0) {
                    remaining_quantity -= added_quantity;
                } else {
                    packs.emplace_back(++pack_number);
                }
            }
        }

        return packs;
    }

public:
    explicit parallel_pack_strategy(int thread_count = 4)
        : m_thread_count(
              std::max(1, std::min(thread_count,
                                   static_cast<int>(std::thread::hardware_concurrency())))) {}

    std::vector<pack> pack_items(const std::vector<item>& items,
                               int max_items,
                               double max_weight) override {
        if (items.empty()) {
            return {};
        }

        // If we have fewer items than threads, just use blocking strategy
        if (items.size() < static_cast<size_t>(m_thread_count) * 10) {
            return pack_chunk(items, max_items, max_weight, 1);
        }

        // Calculate chunk size
        size_t chunk_size = items.size() / m_thread_count;
        if (chunk_size == 0) chunk_size = 1;

        // Create futures for parallel processing
        std::vector<std::future<std::vector<pack>>> futures;
        futures.reserve(m_thread_count);

        // Atomic counter for pack numbering across threads
        std::atomic<int> global_pack_counter{1};

        for (int i = 0; i < m_thread_count; ++i) {
            size_t start_idx = i * chunk_size;
            size_t end_idx = (i == m_thread_count - 1) ? items.size() : (i + 1) * chunk_size;

            if (start_idx >= items.size()) break;

            // Create chunk
            std::vector<item> chunk(items.begin() + start_idx, items.begin() + end_idx);

            // Reserve pack numbers for this chunk (rough estimate)
            int estimated_packs = std::max(1, static_cast<int>(chunk.size() / 50));
            int start_pack_num = global_pack_counter.fetch_add(estimated_packs);

            // Launch async task
            futures.emplace_back(std::async(std::launch::async,
                [this, chunk = std::move(chunk), max_items, max_weight, start_pack_num]() mutable {
                    return pack_chunk(chunk, max_items, max_weight, start_pack_num);
                }));
        }

        // Collect results
        std::vector<pack> all_packs;
        size_t total_estimated_packs = 0;

        // First pass: estimate total size
        for (auto& future : futures) {
            total_estimated_packs += items.size() / 50; // rough estimate
        }
        all_packs.reserve(total_estimated_packs);

        // Second pass: collect actual results
        for (auto& future : futures) {
            auto chunk_packs = future.get();
            all_packs.insert(all_packs.end(),
                           std::make_move_iterator(chunk_packs.begin()),
                           std::make_move_iterator(chunk_packs.end()));
        }

        // Sort packs by pack number to maintain order
        std::sort(all_packs.begin(), all_packs.end(),
                 [](const pack& a, const pack& b) {
                     return a.get_pack_number() < b.get_pack_number();
                 });

        // Renumber packs to be sequential
        int pack_number = 1;
        for (auto& p : all_packs) {
            if (!p.is_empty()) {
                // We need to create a new pack with the correct number
                // Since pack doesn't have a setter, we'll reconstruct
                pack new_pack(pack_number++);
                for (const auto& item : p.get_items()) {
                    new_pack.add_item(item, max_items, max_weight);
                }
                p = std::move(new_pack);
            }
        }

        return all_packs;
    }

    std::string get_name() const override {
        return "Parallel(" + std::to_string(m_thread_count) + " threads)";
    }
};
