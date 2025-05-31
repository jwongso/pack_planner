#ifndef PARALLEL_PACK_PLANNER_H
#define PARALLEL_PACK_PLANNER_H

#include <thread>
#include <future>
#include <mutex>
#include <vector>
#include <algorithm>
#include "pack_planner.h"

namespace pplanner {

class parallel_pack_planner : public pack_planner {
private:
    std::mutex m_packs_mutex;
    unsigned m_thread_count;

public:
    parallel_pack_planner(int max_items, double max_weight, sort_order order,
                         bool enable_timing = true, int max_packs = 99999,
                         unsigned thread_count = std::thread::hardware_concurrency())
        : pack_planner(max_items, max_weight, order, enable_timing, max_packs),
          m_thread_count(thread_count ? thread_count : 2) {}

    void parallel_pack_items() {
        m_packs.clear();
        std::vector<std::future<void>> futures;
        const size_t chunk_size = m_items.size() / m_thread_count + 1;

        auto worker = [this](auto begin, auto end) {
            std::vector<pack> local_packs;
            int pack_id = 1;

            for (auto it = begin; it != end; ++it) {
                const auto& item_obj = *it;
                int remaining_quantity = item_obj.get_quantity();

                while (remaining_quantity > 0) {
                    pack* suitable_pack = find_best_fit_pack(local_packs, item_obj);

                    if (!suitable_pack) {
                        local_packs.emplace_back(pack_id++, this->m_max_items, this->m_max_weight);
                        suitable_pack = &local_packs.back();
                    }

                    int max_possible = calculate_max_possible_quantity(*suitable_pack, item_obj, remaining_quantity);
                    if (max_possible > 0) {
                        suitable_pack->add_item(item_obj.with_quantity(max_possible));
                        remaining_quantity -= max_possible;
                    } else {
                        throw std::runtime_error("Packing error in parallel worker");
                    }
                }
            }

            std::lock_guard<std::mutex> lock(m_packs_mutex);
            m_packs.insert(m_packs.end(), local_packs.begin(), local_packs.end());
        };

        auto chunk_start = m_items.begin();
        for (unsigned i = 0; i < m_thread_count; ++i) {
            auto chunk_end = (i == m_thread_count - 1)
                          ? m_items.end()
                          : std::min(chunk_start + chunk_size, m_items.end());

            futures.emplace_back(std::async(std::launch::async, [=]() {
                worker(chunk_start, chunk_end);
            }));
            chunk_start = chunk_end;
        }

        for (auto& f : futures) f.get();
        consolidate_packs();
    }

private:
    pack* find_best_fit_pack(std::vector<pack>& packs, const item& item_obj) {
        pack* best_pack = nullptr;
        double min_waste = std::numeric_limits<double>::max();

        for (auto& pack_obj : packs) {
            if (can_pack_fit_item(pack_obj, item_obj)) {
                double remaining_weight = pack_obj.get_max_weight() - pack_obj.get_current_weight();
                double waste = remaining_weight - item_obj.get_weight();

                if (waste < min_waste) {
                    min_waste = waste;
                    best_pack = &pack_obj;
                }
            }
        }
        return best_pack;
    }

    bool can_pack_fit_item(const pack& pack_obj, const item& item_obj) const {
        double remaining_weight = pack_obj.get_max_weight() - pack_obj.get_current_weight();
        int remaining_slots = pack_obj.get_max_items() - pack_obj.get_current_item_count();

        return (remaining_weight + WEIGHT_EPSILON >= item_obj.get_weight()) &&
               (remaining_slots > 0);
    }

    int calculate_max_possible_quantity(const pack& p, const item& item_obj, int remaining_quantity) const {
        double remaining_weight = p.get_max_weight() - p.get_current_weight();
        int max_by_weight = static_cast<int>(remaining_weight / item_obj.get_weight());
        int max_by_slots = p.get_max_items() - p.get_current_item_count();

        return std::min({remaining_quantity, max_by_weight, max_by_slots});
    }

    void consolidate_packs() {
        if (m_packs.size() <= 1) return;

        std::vector<pack> consolidated;
        consolidated.reserve(m_packs.size());

        std::sort(m_packs.begin(), m_packs.end(),
            [](const pack& a, const pack& b) {
                double a_remaining = a.get_max_weight() - a.get_current_weight();
                double b_remaining = b.get_max_weight() - b.get_current_weight();
                return a_remaining > b_remaining;
            });

        for (auto& pack_obj : m_packs) {
            bool merged = false;
            for (auto& target : consolidated) {
                if (can_merge_packs(target, pack_obj)) {
                    merge_packs(target, pack_obj);
                    merged = true;
                    break;
                }
            }
            if (!merged) consolidated.push_back(std::move(pack_obj));
        }

        m_packs = std::move(consolidated);
    }

    bool can_merge_packs(const pack& target, const pack& source) const {
        double combined_weight = target.get_current_weight() + source.get_current_weight();
        int combined_items = target.get_current_item_count() + source.get_current_item_count();

        return (combined_weight <= target.get_max_weight() + WEIGHT_EPSILON) &&
               (combined_items <= target.get_max_items());
    }

    void merge_packs(pack& target, pack& source) {
        for (const auto& item : source.get_items()) {
            target.add_item(item);
        }
    }
};

} // namespace pplanner

#endif // PARALLEL_PACK_PLANNER_H
