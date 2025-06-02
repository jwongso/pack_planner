#pragma once

#include <vector>
#include <string>
#include <iostream>
#include "Item.h"
#include "Pack.h"
#include "SortOrder.h"
#include "Timer.h"

struct PackPlannerConfig {
    SortOrder sortOrder;
    int maxItemsPerPack;
    double maxWeightPerPack;
};

struct PackPlannerResult {
    std::vector<Pack> packs;
    double sortingTime;
    double packingTime;
    double totalTime;
    int totalItems;
    double utilizationPercent;
};

class PackPlanner {
public:
    PackPlanner();
    
    // Parse input from stream
    bool parseInput(std::istream& input, PackPlannerConfig& config, std::vector<Item>& items);
    
    // Plan packs with given configuration and items
    PackPlannerResult planPacks(const PackPlannerConfig& config, std::vector<Item> items);
    
    // Output results
    void outputResults(const std::vector<Pack>& packs, std::ostream& output = std::cout);
    
    // Calculate utilization percentage
    double calculateUtilization(const std::vector<Pack>& packs, double maxWeight) const;

private:
    // Sort items according to sort order
    void sortItems(std::vector<Item>& items, SortOrder order);
    
    // Pack items into packs
    std::vector<Pack> packItems(const std::vector<Item>& items, int maxItems, double maxWeight);
    
    Timer timer_;
};
