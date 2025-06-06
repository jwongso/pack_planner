import createModule from './pack_planner_wasm.js';

const Module = await createModule(); // Top-level await is OK in .mjs
const planner = new Module.PackPlanner();

const items = [
  [1, 10, 1, 2.5],
  [2, 5, 1, 1.0],
  [3, 15, 1, 3.0],
];

const rawPacks = planner.packItems(items, 10, 10.0, 0, 0, 4);

// Manual conversion
const packs = [];
for (let i = 0; i < rawPacks.size(); i++) {
  packs.push(rawPacks.get(i));
}

console.log("📦 Packs:", packs);

// Assertions
if (!Array.isArray(packs)) throw new Error("Expected packItems to return an array.");


const stats = planner.getPlanningStats(items, 10, 10.0, 0, 0, 4);
console.log("\n📊 Stats:", {
  sortingTime: stats.sortingTime,
  packingTime: stats.packingTime,
  totalTime: stats.totalTime,
  totalItems: stats.totalItems,
  utilizationPercent: stats.utilizationPercent,
  strategyName: stats.strategyName,
  packCount: stats.packCount,
});

// Quick assertions
if (typeof stats.totalItems !== 'number') throw new Error("Invalid stats object.");

console.log("\n✅ All checks passed.");
