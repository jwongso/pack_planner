#include "PackPlanner.h"
#include <sstream>
#include <algorithm>
#include <iomanip>

PackPlanner::PackPlanner() {
}

bool PackPlanner::parseInput(std::istream& input, PackPlannerConfig& config, std::vector<Item>& items) {
    std::string line;

    // Parse first line: sort order, max items, max weight
    if (!std::getline(input, line) || line.empty()) {
        return false;
    }

    std::istringstream firstLine(line);
    std::string sortOrderStr, maxItemsStr, maxWeightStr;

    if (!std::getline(firstLine, sortOrderStr, ',') ||
        !std::getline(firstLine, maxItemsStr, ',') ||
        !std::getline(firstLine, maxWeightStr)) {
        return false;
    }

    config.sortOrder = parseSortOrder(sortOrderStr);
    config.maxItemsPerPack = std::stoi(maxItemsStr);
    config.maxWeightPerPack = std::stod(maxWeightStr);

    // Parse items
    while (std::getline(input, line) && !line.empty()) {
        std::istringstream itemLine(line);
        std::string idStr, lengthStr, quantityStr, weightStr;

        if (std::getline(itemLine, idStr, ',') &&
            std::getline(itemLine, lengthStr, ',') &&
            std::getline(itemLine, quantityStr, ',') &&
            std::getline(itemLine, weightStr)) {

            int id = std::stoi(idStr);
            int length = std::stoi(lengthStr);
            int quantity = std::stoi(quantityStr);
            double weight = std::stod(weightStr);

            items.emplace_back(id, length, quantity, weight);
        }
    }

    return true;
}

PackPlannerResult PackPlanner::planPacks(const PackPlannerConfig& config, std::vector<Item> items) {
    PackPlannerResult result;

    // Start total timing
    timer_.start();

    // Sort items
    Timer sortTimer;
    sortTimer.start();
    sortItems(items, config.sortOrder);
    result.sortingTime = sortTimer.stop();

    // Pack items
    Timer packTimer;
    packTimer.start();
    result.packs = packItems(items, config.maxItemsPerPack, config.maxWeightPerPack);
    result.packingTime = packTimer.stop();

    result.totalTime = timer_.stop();

    // Calculate total items
    result.totalItems = 0;
    for (const auto& item : items) {
        result.totalItems += item.getQuantity();
    }

    // Calculate utilization
    result.utilizationPercent = calculateUtilization(result.packs, config.maxWeightPerPack);

    return result;
}

void PackPlanner::outputResults(const std::vector<Pack>& packs, std::ostream& output) {
    for (const auto& pack : packs) {
        if (!pack.isEmpty()) {
            output << pack.toString() << std::endl;
        }
    }
}

double PackPlanner::calculateUtilization(const std::vector<Pack>& packs, double maxWeight) const {
    if (packs.empty()) return 0.0;

    double totalWeight = 0.0;
    int nonEmptyPacks = 0;

    for (const auto& pack : packs) {
        if (!pack.isEmpty()) {
            totalWeight += pack.getTotalWeight();
            nonEmptyPacks++;
        }
    }

    if (nonEmptyPacks == 0) return 0.0;

    double maxPossibleWeight = nonEmptyPacks * maxWeight;
    return (totalWeight / maxPossibleWeight) * 100.0;
}

void PackPlanner::sortItems(std::vector<Item>& items, SortOrder order) {
    switch (order) {
        case SortOrder::SHORT_TO_LONG:
            std::sort(items.begin(), items.end());
            break;
        case SortOrder::LONG_TO_SHORT:
            std::sort(items.begin(), items.end(), std::greater<Item>());
            break;
        case SortOrder::NATURAL:
        default:
            // Keep original order
            break;
    }
}

// std::vector<Pack> PackPlanner::packItems(const std::vector<Item>& items, int maxItems, double maxWeight) {
//     std::vector<Pack> packs;

//     int packNumber = 1;
//     Pack currentPack(packNumber++);

//     for (const auto& item : items) {
//         int remainingQuantity = item.getQuantity();

//         while (remainingQuantity > 0) {
//             // Try to add as much as possible to current pack
//             Item tempItem(item.getId(), item.getLength(), remainingQuantity, item.getWeight());
//             int addedQuantity = currentPack.addPartialItem(tempItem, maxItems, maxWeight);

//             if (addedQuantity > 0) {
//                 remainingQuantity -= addedQuantity;
//             } else {
//                 // Current pack is full, start a new one
//                 if (!currentPack.isEmpty()) {
//                     packs.push_back(currentPack);
//                 }
//                 currentPack = Pack(packNumber++);

//                 // Try again with new pack
//                 addedQuantity = currentPack.addPartialItem(tempItem, maxItems, maxWeight);
//                 if (addedQuantity > 0) {
//                     remainingQuantity -= addedQuantity;
//                 } else {
//                     // This shouldn't happen with valid constraints
//                     break;
//                 }
//             }
//         }
//     }

//     // Add the last pack if it's not empty
//     if (!currentPack.isEmpty()) {
//         packs.push_back(currentPack);
//     }

//     return packs;
// }

std::vector<Pack> PackPlanner::packItems(const std::vector<Item>& items, int maxItems, double maxWeight) {
    std::vector<Pack> packs;
    int packNumber = 1;
    packs.emplace_back(packNumber);

    for (const auto& item : items) {
        int remainingQuantity = item.getQuantity();

        while (remainingQuantity > 0) {
            // Try to add as much as possible to current pack
            Pack& currentPack = packs.back();
            Item tempItem(item.getId(), item.getLength(), remainingQuantity, item.getWeight());
            int addedQuantity = currentPack.addPartialItem(tempItem, maxItems, maxWeight);

            if (addedQuantity > 0) {
                remainingQuantity -= addedQuantity;
            } else {
                // Current pack is full, start a new one
                packs.emplace_back(packNumber++);
                Pack& currentPack = packs.back();
                // Try again with new pack~
                addedQuantity = currentPack.addPartialItem(tempItem, maxItems, maxWeight);
                if (addedQuantity > 0) {
                    remainingQuantity -= addedQuantity;
                } else {
                    // This shouldn't happen with valid constraints
                    break;
                }
            }
        }
    }

    return packs;
}
