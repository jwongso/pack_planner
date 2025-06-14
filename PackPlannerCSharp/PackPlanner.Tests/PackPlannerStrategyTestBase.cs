using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Threading;
using PackPlanner;
using Xunit;

namespace PackPlanner.Tests
{
    /// <summary>
    /// Base class for all packing strategy tests (shared fixtures and helpers).
    /// </summary>
    public abstract class PackPlannerStrategyTestBase : IDisposable
    {
        protected readonly PackPlanner _planner;
        protected readonly List<Item> _items;
        protected PackPlannerConfig _config;
        protected readonly CancellationTokenSource _cts;

        protected PackPlannerStrategyTestBase(StrategyType strategyType)
        {
            _cts = new CancellationTokenSource();
            _planner = new PackPlanner();

            // Standard test items
            _items = new List<Item>
            {
                new Item(1, 100, 5, 2.0),  // Total weight: 10.0
                new Item(2, 200, 3, 3.0),  // Total weight: 9.0
                new Item(3, 300, 2, 5.0),  // Total weight: 10.0
                new Item(4, 150, 4, 2.5)   // Total weight: 10.0
            };

            // Base configuration
            _config = new PackPlannerConfig
            {
                Order = SortOrder.Natural,
                MaxItemsPerPack = 10,
                MaxWeightPerPack = 25.0,
                Type = strategyType,
                ThreadCount = strategyType == StrategyType.Parallel ? 4 : 1
            };
        }

        public void Dispose() => _cts?.Dispose();

        // --- Shared Test Helpers ---
        protected long TimeExecution(Action action)
        {
            var sw = Stopwatch.StartNew();
            action();
            sw.Stop();
            return sw.ElapsedMilliseconds;
        }

        protected void AssertPacksMeetConstraints(PackPlannerResult result)
        {
            foreach (var pack in result.Packs.Where(p => !p.IsEmpty))
            {
                Assert.True(pack.TotalItems <= _config.MaxItemsPerPack);
                Assert.True(pack.TotalWeight <= _config.MaxWeightPerPack);
            }
        }
    }
}
