// Pack Planner - Adaptive Processing Application
// Combines WASM and Web API functionality with intelligent runtime decision making

// Global state management
class AppState {
    constructor() {
        this.wasmModule = null;
        this.systemProfiler = null;
        this.currentProcessingMode = 'auto';
        this.apiConfig = {
            baseUrl: 'http://localhost:5135/api',
            timeout: 300000,
            retryAttempts: 3
        };
        this.fullInputData = null;
        this.fullResultsData = null;
        this.benchmarkResults = [];
        this.currentJobId = null;
        this.jobPollingInterval = null;
        this.systemProfile = null;
    }
}

const appState = new AppState();

// System Profiler Integration
class SystemProfiler {
    constructor() {
        this.networkLatency = 0;
        this.networkBandwidth = 0;
        this.batteryLevel = 1.0;
        this.isCharging = true;
        this.cpuScore = 0;
        this.memoryBandwidth = 0;
    }

    async initialize() {
        await this.initializeBatteryAPI();
        await this.runNetworkTest();
        if (appState.wasmModule && appState.systemProfiler) {
            this.runCPUMemoryTest();
        }
    }

    async initializeBatteryAPI() {
        if ('getBattery' in navigator) {
            try {
                const battery = await navigator.getBattery();
                this.batteryLevel = battery.level;
                this.isCharging = battery.charging;

                battery.addEventListener('levelchange', () => {
                    this.batteryLevel = battery.level;
                });

                battery.addEventListener('chargingchange', () => {
                    this.isCharging = battery.charging;
                });

                console.log(`Battery API initialized: ${(this.batteryLevel * 100).toFixed(1)}%, ${this.isCharging ? 'charging' : 'not charging'}`);
            } catch (error) {
                console.log(`Battery API not available: ${error.message}`);
            }
        }
    }

    async runNetworkTest() {
        try {
            const endpoints = [
                'https://httpbin.org/delay/0',
                'https://jsonplaceholder.typicode.com/posts/1',
                'https://api.github.com'
            ];

            let totalLatency = 0;
            let successfulTests = 0;

            for (const endpoint of endpoints) {
                try {
                    const start = performance.now();
                    const response = await fetch(endpoint, { method: 'HEAD', timeout: 5000 });
                    const end = performance.now();

                    if (response.ok) {
                        totalLatency += (end - start);
                        successfulTests++;
                    }
                } catch (e) {
                    console.log(`Failed to test ${endpoint}: ${e.message}`);
                }
            }

            if (successfulTests > 0) {
                this.networkLatency = totalLatency / successfulTests;
            }

            // Test bandwidth with smaller payload for faster results
            try {
                const start = performance.now();
                const response = await fetch('https://httpbin.org/bytes/262144'); // 256KB
                const data = await response.arrayBuffer();
                const end = performance.now();

                const duration = (end - start) / 1000;
                const bytes = data.byteLength;
                this.networkBandwidth = (bytes / (1024 * 1024)) / duration;
            } catch (e) {
                console.log(`Bandwidth test failed: ${e.message}`);
                this.networkBandwidth = 1; // Default fallback
            }
        } catch (error) {
            console.log(`Network test error: ${error.message}`);
            this.networkLatency = 100; // Default fallback
            this.networkBandwidth = 1;
        }
    }

    runCPUMemoryTest() {
        try {
            const results = appState.systemProfiler.profileSystem();
            this.cpuScore = results.cpuScore;
            this.memoryBandwidth = results.memoryBandwidthMbps;
        } catch (error) {
            console.log(`CPU/Memory test failed: ${error.message}`);
            this.cpuScore = 5; // Default fallback
            this.memoryBandwidth = 1000;
        }
    }

    calculateRecommendation() {
        const weights = {
            cpu: 0.3,
            memory: 0.25,
            networkLatency: 0.2,
            networkBandwidth: 0.15,
            battery: 0.1
        };

        const cpuNorm = Math.min(this.cpuScore / 10.0, 1.0);
        const memoryNorm = Math.min(this.memoryBandwidth / 10000.0, 1.0);
        const latencyNorm = Math.max(0, 1.0 - (this.networkLatency / 1000.0));
        const bandwidthNorm = Math.min(this.networkBandwidth / 100.0, 1.0);
        const batteryNorm = this.batteryLevel;

        const overallScore =
            cpuNorm * weights.cpu +
            memoryNorm * weights.memory +
            latencyNorm * weights.networkLatency +
            bandwidthNorm * weights.networkBandwidth +
            batteryNorm * weights.battery;

        let decision;
        if (overallScore > 0.7) {
            decision = 'wasm';
        } else if (overallScore > 0.4) {
            decision = 'hybrid';
        } else {
            decision = 'api';
        }

        return {
            decision: decision,
            confidence: overallScore,
            details: {
                cpuScore: this.cpuScore,
                memoryBandwidth: this.memoryBandwidth,
                networkLatency: this.networkLatency,
                networkBandwidth: this.networkBandwidth,
                batteryLevel: this.batteryLevel,
                isCharging: this.isCharging
            }
        };
    }

    getProfileSummary() {
        return {
            cpuScore: this.cpuScore.toFixed(2),
            memoryBandwidth: this.memoryBandwidth.toFixed(2),
            networkLatency: this.networkLatency.toFixed(2),
            networkBandwidth: this.networkBandwidth.toFixed(2),
            batteryLevel: (this.batteryLevel * 100).toFixed(1),
            isCharging: this.isCharging
        };
    }
}

const systemProfiler = new SystemProfiler();

// WASM Module Loader
class WASMLoader {
    static async loadModule() {
        try {
            updateStatus('Loading WASM module...', 'loading');
            
            const wasmModule = await import('./pack_planner_wasm.js');
            appState.wasmModule = await wasmModule.default();
            appState.systemProfiler = new appState.wasmModule.SystemProfiler();
            
            console.log('WASM module loaded successfully');
            return true;
        } catch (error) {
            console.error('WASM loading error:', error);
            return false;
        }
    }
}

// API Client
class APIClient {
    static async testConnection() {
        const statusEl = document.getElementById('connectionStatus');
        const testButton = document.getElementById('testConnectionButton');

        try {
            statusEl.innerHTML = '<div class="spinner" style="width: 16px; height: 16px;"></div>';
            testButton.disabled = true;

            const baseUrl = document.getElementById('apiBaseUrl').value;
            appState.apiConfig.baseUrl = baseUrl;

            const response = await fetch(`${baseUrl}/health`, {
                method: 'GET',
                headers: { 'Accept': 'application/json' }
            });

            if (response.ok) {
                const health = await response.json();
                statusEl.innerHTML = '✅ Connected';
                statusEl.style.color = '#28a745';
                return true;
            } else {
                throw new Error(`HTTP ${response.status}: ${response.statusText}`);
            }
        } catch (error) {
            statusEl.innerHTML = '❌ Failed';
            statusEl.style.color = '#dc3545';
            console.error('API connection failed:', error);
            return false;
        } finally {
            testButton.disabled = false;
        }
    }

    static prepareApiRequest() {
        const maxItems = parseInt(document.getElementById('maxItems').value);
        const maxWeight = parseFloat(document.getElementById('maxWeight').value);
        const sortOrder = document.getElementById('sortOrder').value;
        const strategyType = document.getElementById('strategyType').value;
        const threadCount = parseInt(document.getElementById('threadCount').value);

        let items;
        if (appState.fullInputData && appState.fullInputData.length > 0) {
            items = appState.fullInputData;
        } else {
            const itemsText = document.getElementById('itemsInput').value.trim();
            items = JSON.parse(itemsText);
        }

        return {
            items: items.map(item => ({
                id: item[0],
                length: item[1],
                quantity: item[2],
                weight: item[3]
            })),
            configuration: {
                sortOrder: ['NATURAL', 'SHORT_TO_LONG', 'LONG_TO_SHORT'][parseInt(sortOrder)],
                maxItemsPerPack: maxItems,
                maxWeightPerPack: maxWeight,
                strategyType: parseInt(strategyType) === 0 ? 'BLOCKING' : 'PARALLEL',
                threadCount: threadCount
            }
        };
    }

    static async runSyncPacking(requestData) {
        const response = await fetch(`${appState.apiConfig.baseUrl}/pack`, {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
                'Accept': 'application/json'
            },
            body: JSON.stringify(requestData)
        });

        if (!response.ok) {
            const errorData = await response.json();
            throw new Error(errorData.error || `HTTP ${response.status}`);
        }

        return await response.json();
    }

    static async runAsyncPacking(requestData) {
        const response = await fetch(`${appState.apiConfig.baseUrl}/pack/async`, {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
                'Accept': 'application/json'
            },
            body: JSON.stringify(requestData)
        });

        if (!response.ok) {
            const errorData = await response.json();
            throw new Error(errorData.error || `HTTP ${response.status}`);
        }

        const jobResponse = await response.json();
        appState.currentJobId = jobResponse.jobId;
        return jobResponse;
    }

    static async runBenchmark(config) {
        const response = await fetch(`${appState.apiConfig.baseUrl}/benchmark`, {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
                'Accept': 'application/json'
            },
            body: JSON.stringify(config)
        });

        if (!response.ok) {
            const errorData = await response.json();
            throw new Error(errorData.error || `HTTP ${response.status}`);
        }

        return await response.json();
    }
}

// WASM Client
class WASMClient {
    static runPacking() {
        const maxItems = parseInt(document.getElementById('maxItems').value);
        const maxWeight = parseFloat(document.getElementById('maxWeight').value);
        const sortOrder = parseInt(document.getElementById('sortOrder').value);
        const strategyType = parseInt(document.getElementById('strategyType').value);
        const threadCount = parseInt(document.getElementById('threadCount').value);

        let items;
        if (appState.fullInputData && appState.fullInputData.length > 0) {
            items = appState.fullInputData;
        } else {
            const itemsText = document.getElementById('itemsInput').value.trim();
            items = JSON.parse(itemsText);
        }

        const planner = new appState.wasmModule.PackPlanner();
        
        // Get planning stats
        const stats = planner.getPlanningStats(
            items, maxItems, maxWeight, sortOrder, strategyType, threadCount
        );

        // Get packed results
        const packedResults = planner.packItems(
            items, maxItems, maxWeight, sortOrder, strategyType, threadCount
        );

        // Clean up
        planner.delete();

        return { stats, packedResults };
    }

    static runBenchmark(size, sortOrder, strategy, threads) {
        const planner = new appState.wasmModule.PackPlanner();
        const result = planner.runBenchmark(size, sortOrder, strategy, threads);
        planner.delete();
        return result;
    }
}

// Processing Mode Manager
class ProcessingModeManager {
    static async determineProcessingMode() {
        const selectedMode = document.querySelector('input[name="processingMode"]:checked').value;
        
        if (selectedMode === 'wasm') {
            return 'wasm';
        } else if (selectedMode === 'api') {
            return 'api';
        } else {
            // Auto mode - use profiler
            const recommendation = systemProfiler.calculateRecommendation();
            appState.systemProfile = recommendation;
            
            // Update UI with profiler results
            this.updateProfilerDisplay(recommendation);
            
            if (recommendation.decision === 'hybrid') {
                // For hybrid, prefer WASM if available, otherwise API
                return appState.wasmModule ? 'wasm' : 'api';
            }
            
            return recommendation.decision;
        }
    }

    static updateProfilerDisplay(recommendation) {
        const profilerResults = document.getElementById('profilerResults');
        const profilerOutput = document.getElementById('profilerOutput');
        
        const profile = systemProfiler.getProfileSummary();
        
        profilerOutput.innerHTML = `
            <div><strong>CPU Score:</strong> ${profile.cpuScore} MOPS</div>
            <div><strong>Memory Bandwidth:</strong> ${profile.memoryBandwidth} MB/s</div>
            <div><strong>Network Latency:</strong> ${profile.networkLatency} ms</div>
            <div><strong>Network Bandwidth:</strong> ${profile.networkBandwidth} MB/s</div>
            <div><strong>Battery:</strong> ${profile.batteryLevel}% ${profile.isCharging ? '(charging)' : '(not charging)'}</div>
            <div><strong>Recommendation:</strong> ${recommendation.decision.toUpperCase()}</div>
            <div><strong>Confidence:</strong> ${(recommendation.confidence * 100).toFixed(1)}%</div>
        `;
        
        profilerResults.style.display = 'block';
    }

    static updateCurrentModeDisplay(mode) {
        const currentModeText = document.getElementById('currentModeText');
        const modeNames = {
            'wasm': '⚡ WASM (Client-side)',
            'api': '🌐 Web API (Server-side)',
            'detecting': '🔍 Detecting...'
        };
        
        currentModeText.textContent = modeNames[mode] || mode;
        appState.currentProcessingMode = mode;
        
        // Show/hide API configuration based on mode
        const apiConfig = document.getElementById('apiConfig');
        const asyncControls = document.getElementById('asyncControls');
        
        if (mode === 'api') {
            apiConfig.style.display = 'block';
            asyncControls.style.display = 'block';
        } else {
            apiConfig.style.display = 'none';
            asyncControls.style.display = 'none';
        }
    }
}

// UI Helper Functions
function updateStatus(message, type = 'loading') {
    const statusEl = document.getElementById('status');
    statusEl.textContent = message;
    statusEl.className = `status ${type}`;
}

function updateProgress(percentage) {
    const progressBar = document.getElementById('progressBar');
    const progressFill = document.getElementById('progressFill');
    progressBar.style.display = 'block';
    progressFill.style.width = `${percentage}%`;
}

function updateStep(stepNumber, status) {
    const stepIcon = document.getElementById(`step${stepNumber}Icon`);
    stepIcon.className = `step-icon step-${status}`;
    if (status === 'complete') {
        stepIcon.innerHTML = '✓';
    } else if (status === 'error') {
        stepIcon.innerHTML = '✗';
    } else if (status === 'active') {
        stepIcon.innerHTML = '<div class="spinner" style="width: 12px; height: 12px; border-width: 2px; margin: 0;"></div>';
    } else {
        stepIcon.innerHTML = stepNumber;
    }
}

function showProcessingSteps() {
    const processingSteps = document.getElementById('processingSteps');
    processingSteps.style.display = 'block';
    for (let i = 1; i <= 5; i++) {
        updateStep(i, 'pending');
    }
}

function hideProcessingSteps() {
    const processingSteps = document.getElementById('processingSteps');
    const progressBar = document.getElementById('progressBar');
    processingSteps.style.display = 'none';
    progressBar.style.display = 'none';
}

// Data Generation Functions
function generateTestData() {
    const itemCount = parseInt(document.getElementById('itemCount').value);

    if (itemCount < 1 || itemCount > 20000000) {
        document.getElementById('error').innerHTML =
            '<div class="error">❌ Item count must be between 1 and 20,000,000</div>';
        return;
    }

    const sizes = [50, 100, 150, 200, 250, 300, 350, 400];
    const items = [];

    const statusEl = document.getElementById('status');
    const generateButton = document.getElementById('generateButton');

    if (itemCount > 1000) {
        statusEl.textContent = `🎲 Generating ${itemCount.toLocaleString()} items...`;
        statusEl.className = 'status loading';
        generateButton.disabled = true;
    }

    const batchSize = 10000;
    let processed = 0;

    function generateBatch() {
        const endIndex = Math.min(processed + batchSize, itemCount);

        for (let i = processed; i < endIndex; i++) {
            const id = i + 1;
            const length = sizes[Math.floor(Math.random() * sizes.length)];
            const quantity = Math.floor(Math.random() * 10) + 1;
            const weight = (Math.random() * 4.5 + 0.5).toFixed(1);

            items.push([id, length, quantity, parseFloat(weight)]);
        }

        processed = endIndex;

        if (processed < itemCount) {
            if (itemCount > 1000) {
                statusEl.textContent = `🎲 Generating items... ${processed.toLocaleString()}/${itemCount.toLocaleString()}`;
            }
            setTimeout(generateBatch, 0);
        } else {
            appState.fullInputData = [...items];
            const displayItems = items.slice(0, 100);
            const displayData = JSON.stringify(displayItems, null, 2);

            if (items.length > 100) {
                document.getElementById('itemsInput').value = displayData +
                    `\n\n// Showing first 100 of ${items.length.toLocaleString()} items\n// Use "Download Full Input Data" to get complete dataset`;
            } else {
                document.getElementById('itemsInput').value = displayData;
            }

            document.getElementById('downloadInputButton').disabled = items.length <= 100;

            if (itemCount > 1000) {
                statusEl.textContent = `✅ Generated ${itemCount.toLocaleString()} items successfully!`;
                statusEl.className = 'status success';
                generateButton.disabled = false;
            }

            document.getElementById('error').innerHTML = '';
        }
    }

    generateBatch();
}

// Download Functions
function downloadInputData() {
    if (!appState.fullInputData || appState.fullInputData.length === 0) {
        alert('No full input data available to download');
        return;
    }

    const jsonData = JSON.stringify(appState.fullInputData, null, 2);
    const blob = new Blob([jsonData], { type: 'application/json' });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = `pack_planner_input_${appState.fullInputData.length}_items.json`;
    document.body.appendChild(a);
    a.click();
    document.body.removeChild(a);
    URL.revokeObjectURL(url);
}

function downloadResults() {
    if (!appState.fullResultsData || appState.fullResultsData.length === 0) {
        alert('No full results data available to download');
        return;
    }

    const textData = appState.fullResultsData.join('\n\n' + '='.repeat(50) + '\n\n');
    const blob = new Blob([textData], { type: 'text/plain' });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = `pack_planner_results_${appState.fullResultsData.length}_packs.txt`;
    document.body.appendChild(a);
    a.click();
    document.body.removeChild(a);
    URL.revokeObjectURL(url);
}

// Result Display Functions
function displayWASMResults(result, processingTime) {
    const resultsEl = document.getElementById('results');
    const { stats, packedResults } = result;

    if (packedResults && packedResults.size() > 0) {
        appState.fullResultsData = [];
        for (let i = 0; i < packedResults.size(); i++) {
            appState.fullResultsData.push(`Pack ${i + 1}:\n${packedResults.get(i)}`);
        }

        const totalPacks = packedResults.size();
        const packsToShow = Math.min(100, totalPacks);

        let summaryHtml = `
            <div style="background: #e8f5e8; padding: 15px; border-radius: 4px; margin-bottom: 20px;">
                <strong>📊 Summary (WASM):</strong><br>
                • Total Packs: <strong>${totalPacks.toLocaleString()}</strong><br>
                • Strategy: <strong>${stats.strategyName}</strong><br>
                • Sorting Time: <strong>${stats.sortingTime.toFixed(3)}ms</strong><br>
                • Packing Time: <strong>${stats.packingTime.toFixed(3)}ms</strong><br>
                • Total Time: <strong>${stats.totalTime.toFixed(3)}ms</strong><br>
                • JS Processing Time: <strong>${processingTime.toFixed(2)}ms</strong><br>
                • Utilization: <strong>${stats.utilizationPercent.toFixed(2)}%</strong><br>
                • Showing: <strong>First ${packsToShow} pack${packsToShow !== 1 ? 's' : ''}</strong>
            </div>
        `;

        if (totalPacks > 100) {
            summaryHtml += `
                <div style="background: #fff3cd; padding: 10px; border-radius: 4px; margin-bottom: 15px;">
                    📄 <strong>Note:</strong> Showing first 100 of ${totalPacks.toLocaleString()} packs.
                    <button id="downloadResultsButton" style="margin-left: 10px; padding: 5px 10px; font-size: 14px;">📥 Download Full Results</button>
                </div>
            `;
        }

        summaryHtml += `<h3>📦 Pack Details:</h3>`;
        resultsEl.innerHTML = summaryHtml;

        for (let i = 0; i < packsToShow; i++) {
            const packDiv = document.createElement('div');
            packDiv.className = 'pack';
            const packData = packedResults.get(i);
            packDiv.innerHTML = `<strong>Pack ${i + 1}:</strong><br>${packData}`;
            resultsEl.appendChild(packDiv);
        }

        const downloadResultsBtn = document.getElementById('downloadResultsButton');
        if (downloadResultsBtn) {
            downloadResultsBtn.addEventListener('click', downloadResults);
        }

        updateStatus(`✅ Success! Created ${totalPacks.toLocaleString()} packs in ${processingTime.toFixed(2)}ms (WASM)`, 'success');
    } else {
        resultsEl.innerHTML = `
            <div style="background: #fff3cd; padding: 15px; border-radius: 4px;">
                ⚠️ No packs were created.
            </div>
        `;
        updateStatus('⚠️ Completed (no packs created)', 'error');
    }
}

function displayAPIResults(result, processingTime) {
    const resultsEl = document.getElementById('results');

    if (result.success && result.packs && result.packs.length > 0) {
        const totalPacks = result.packs.length;
        const packsToShow = Math.min(100, totalPacks);

        appState.fullResultsData = result.packs.map(pack => {
            return `Pack ${pack.packNumber}:\n` +
                pack.items.map(item =>
                    `${item.id},${item.length},${item.quantity},${item.weight}`
                ).join('\n') +
                `\nPack Length: ${pack.packLength}, Pack Weight: ${pack.totalWeight.toFixed(2)}`;
        });

        let summaryHtml = `
            <div style="background: #e8f5e8; padding: 15px; border-radius: 4px; margin-bottom: 20px;">
                <strong>📊 Summary (Web API):</strong><br>
                • Total Packs: <strong>${totalPacks.toLocaleString()}</strong><br>
                • Strategy: <strong>${result.metrics.strategyUsed}</strong><br>
                • Sorting Time: <strong>${result.metrics.sortingTimeMs.toFixed(3)}ms</strong><br>
                • Packing Time: <strong>${result.metrics.packingTimeMs.toFixed(3)}ms</strong><br>
                • Total Time: <strong>${result.metrics.totalTimeMs.toFixed(3)}ms</strong><br>
                • JS Processing Time: <strong>${processingTime.toFixed(2)}ms</strong><br>
                • Input Items: <strong>${result.metrics.totalItems.toLocaleString()}</strong><br>
                • Utilization: <strong>${result.metrics.utilizationPercent.toFixed(2)}%</strong><br>
                • Showing: <strong>First ${packsToShow} pack${packsToShow !== 1 ? 's' : ''}</strong>
            </div>
        `;

        if (totalPacks > 100) {
            summaryHtml += `
                <div style="background: #fff3cd; padding: 10px; border-radius: 4px; margin-bottom: 15px;">
                    📄 <strong>Note:</strong> Showing first 100 of ${totalPacks.toLocaleString()} packs.
                    <button id="downloadResultsButton" style="margin-left: 10px; padding: 5px 10px; font-size: 14px;">📥 Download Full Results</button>
                </div>
            `;
        }

        summaryHtml += `<h3>📦 Pack Details:</h3>`;
        resultsEl.innerHTML = summaryHtml;

        for (let i = 0; i < packsToShow; i++) {
            const pack = result.packs[i];
            const packDiv = document.createElement('div');
            packDiv.className = 'pack';

            let packContent = `<strong>Pack ${pack.packNumber}:</strong><br>`;
            pack.items.forEach(item => {
                packContent += `${item.id},${item.length},${item.quantity},${item.weight}<br>`;
            });
            packContent += `Pack Length: ${pack.packLength}, Pack Weight: ${pack.totalWeight.toFixed(2)}`;

            packDiv.innerHTML = packContent;
            resultsEl.appendChild(packDiv);
        }

        const downloadResultsBtn = document.getElementById('downloadResultsButton');
        if (downloadResultsBtn) {
            downloadResultsBtn.addEventListener('click', downloadResults);
        }

        updateStatus(`✅ Success! Created ${totalPacks.toLocaleString()} packs in ${processingTime.toFixed(2)}ms (Web API)`, 'success');
    } else {
        resultsEl.innerHTML = `
            <div style="background: #fff3cd; padding: 15px; border-radius: 4px;">
                ⚠️ No packs were created or API returned unexpected format.
            </div>
        `;
        updateStatus('⚠️ Completed (no packs created)', 'error');
    }
}

// Main Processing Functions
async function runPacking() {
    const errorEl = document.getElementById('error');
    const resultsEl = document.getElementById('results');
    const runButton = document.getElementById('runButton');

    updateStatus('Processing...', 'loading');
    errorEl.innerHTML = '';
    resultsEl.innerHTML = '';
    runButton.disabled = true;

    showProcessingSteps();
    updateProgress(0);

    try {
        // Step 1: Validate inputs
        updateStep(1, 'active');
        await new Promise(resolve => setTimeout(resolve, 100));

        const maxItems = parseInt(document.getElementById('maxItems').value);
        const maxWeight = parseFloat(document.getElementById('maxWeight').value);
        const itemsText = document.getElementById('itemsInput').value.trim();

        if (maxItems <= 0) throw new Error('Max items must be positive');
        if (maxWeight <= 0) throw new Error('Max weight must be positive');
        if (!itemsText && (!appState.fullInputData || appState.fullInputData.length === 0)) {
            throw new Error('Items input is empty');
        }

        updateStep(1, 'complete');
        updateProgress(20);

        // Step 2: Parse item data
        updateStep(2, 'active');
        await new Promise(resolve => setTimeout(resolve, 100));

        let items;
        if (appState.fullInputData && appState.fullInputData.length > 0) {
            items = appState.fullInputData;
        } else {
            items = JSON.parse(itemsText);
        }

        if (!Array.isArray(items) || items.length === 0) {
            throw new Error('Items must be a non-empty array');
        }

        updateStep(2, 'complete');
        updateProgress(40);

        // Step 3: Determine processing mode
        updateStep(3, 'active');
        updateStatus('Determining processing mode...', 'loading');
        await new Promise(resolve => setTimeout(resolve, 100));

        const processingMode = await ProcessingModeManager.determineProcessingMode();
        ProcessingModeManager.updateCurrentModeDisplay(processingMode);

        updateStep(3, 'complete');
        updateProgress(60);

        // Step 4: Process based on determined mode
        updateStep(4, 'active');
        const startTime = performance.now();

        if (processingMode === 'wasm') {
            if (!appState.wasmModule) {
                throw new Error('WASM module not available');
            }
            updateStatus('Processing with WASM...', 'loading');
            const result = WASMClient.runPacking();
            const endTime = performance.now();
            
            updateStep(4, 'complete');
            updateStep(5, 'complete');
            updateProgress(100);
            
            displayWASMResults(result, endTime - startTime);
        } else {
            // API mode
            updateStatus('Processing with Web API...', 'loading');
            const requestData = APIClient.prepareApiRequest();
            
            const packingMode = document.getElementById('packingMode').value;
            if (packingMode === 'async') {
                await runAsyncAPIPacking(requestData);
            } else {
                const result = await APIClient.runSyncPacking(requestData);
                const endTime = performance.now();
                
                updateStep(4, 'complete');
                updateStep(5, 'complete');
                updateProgress(100);
                
                displayAPIResults(result, endTime - startTime);
            }
        }

    } catch (error) {
        // Mark current step as error
        for (let i = 1; i <= 5; i++) {
            const stepIcon = document.getElementById(`step${i}Icon`);
            if (stepIcon.className.includes('step-active')) {
                updateStep(i, 'error');
                break;
            }
        }

        document.getElementById('error').innerHTML = `<div class="error">❌ <strong>Error:</strong> ${error.message}</div>`;
        updateStatus('❌ Processing failed', 'error');
        console.error("Packing error:", error);
    } finally {
        runButton.disabled = false;
        setTimeout(hideProcessingSteps, 3000);
    }
}

// Async API Processing
async function runAsyncAPIPacking(requestData) {
    try {
        const jobResponse = await APIClient.runAsyncPacking(requestData);
        showAsyncJobStatus();
        startJobPolling();
        updateStatus(`✅ Async job started: ${appState.currentJobId}`, 'success');
    } catch (error) {
        throw error;
    }
}

function showAsyncJobStatus() {
    document.getElementById('asyncJobStatus').style.display = 'block';
    document.getElementById('currentJobId').textContent = appState.currentJobId;
    document.getElementById('jobStartTime').textContent = new Date().toLocaleTimeString();
}

function hideAsyncJobStatus() {
    document.getElementById('asyncJobStatus').style.display = 'none';
}

function startJobPolling() {
    if (appState.jobPollingInterval) clearInterval(appState.jobPollingInterval);

    appState.jobPollingInterval = setInterval(async () => {
        try {
            const response = await fetch(`${appState.apiConfig.baseUrl}/pack/status/${appState.currentJobId}`);
            if (!response.ok) throw new Error(`HTTP ${response.status}`);

            const status = await response.json();
            updateJobStatus(status);

            if (status.status === 'Completed' || status.status === 'Failed') {
                clearInterval(appState.jobPollingInterval);
                if (status.status === 'Completed') {
                    await fetchJobResult();
                } else {
                    throw new Error(status.errorMessage || 'Job failed');
                }
            }
        } catch (error) {
            clearInterval(appState.jobPollingInterval);
            document.getElementById('error').innerHTML = `<div class="error">❌ <strong>Error:</strong> ${error.message}</div>`;
            document.getElementById('runButton').disabled = false;
        }
    }, 2000);
}

function updateJobStatus(status) {
    document.getElementById('jobStatusText').textContent = status.status;
    document.getElementById('jobProgressFill').style.width = `${status.progress || 0}%`;

    if (status.estimatedTimeRemaining) {
        const eta = new Date(Date.now() + status.estimatedTimeRemaining * 1000);
        document.getElementById('jobEta').textContent = eta.toLocaleTimeString();
    }

    updateProgress(status.progress || 0);
    if (status.progress >= 70) {
        updateStep(4, 'complete');
        updateStep(5, 'active');
    } else if (status.progress >= 30) {
        updateStep(3, 'complete');
        updateStep(4, 'active');
    }
}

async function fetchJobResult() {
    try {
        const response = await fetch(`${appState.apiConfig.baseUrl}/pack/result/${appState.currentJobId}`);
        if (!response.ok) throw new Error(`HTTP ${response.status}`);

        const result = await response.json();
        displayAPIResults(result, 0);

        document.getElementById('runButton').disabled = false;
        updateStatus('✅ Async job completed successfully!', 'success');
        updateStep(5, 'complete');
        updateProgress(100);
    } catch (error) {
        document.getElementById('error').innerHTML = `<div class="error">❌ <strong>Error:</strong> ${error.message}</div>`;
    } finally {
        setTimeout(hideProcessingSteps, 3000);
    }
}

async function cancelCurrentJob() {
    if (!appState.currentJobId) return;

    try {
        await fetch(`${appState.apiConfig.baseUrl}/pack/job/${appState.currentJobId}`, { method: 'DELETE' });
        clearInterval(appState.jobPollingInterval);
        hideAsyncJobStatus();
        document.getElementById('runButton').disabled = false;
        updateStatus('🚫 Job cancelled', 'error');
        appState.currentJobId = null;
    } catch (error) {
        console.error('Cancel job error:', error);
    } finally {
        setTimeout(hideProcessingSteps, 3000);
    }
}

// Benchmark Functions
async function runSingleBenchmark() {
    const benchStatusEl = document.getElementById('benchmarkStatus');
    const runBenchButton = document.getElementById('runBenchmarkButton');
    const runFullBenchButton = document.getElementById('runFullBenchmarkButton');

    benchStatusEl.innerHTML = '<div class="spinner"></div>Running benchmark...';
    benchStatusEl.className = 'status loading';
    runBenchButton.disabled = true;
    runFullBenchButton.disabled = true;

    try {
        const size = parseInt(document.getElementById('benchSize').value);
        const sortOrder = parseInt(document.getElementById('benchSortOrder').value);
        const strategy = parseInt(document.getElementById('benchStrategy').value);
        const threads = parseInt(document.getElementById('benchThreads').value);

        const processingMode = await ProcessingModeManager.determineProcessingMode();
        const startTime = performance.now();

        let result;
        if (processingMode === 'wasm' && appState.wasmModule) {
            result = WASMClient.runBenchmark(size, sortOrder, strategy, threads);
            result.processingMode = 'WASM';
        } else {
            const config = {
                itemCount: size,
                sortOrder: ['NATURAL', 'SHORT_TO_LONG', 'LONG_TO_SHORT'][sortOrder],
                strategyType: strategy === 0 ? 'BLOCKING' : 'PARALLEL',
                threadCount: threads
            };
            result = await APIClient.runBenchmark(config);
            result.processingMode = 'API';
        }

        const jsTime = performance.now() - startTime;
        result.jsProcessingTime = jsTime;

        addBenchmarkResult(result);

        benchStatusEl.textContent = `✅ Benchmark completed in ${jsTime.toFixed(2)}ms (${result.processingMode})`;
        benchStatusEl.className = 'status success';

    } catch (e) {
        benchStatusEl.innerHTML = `<div class="error">❌ <strong>Error:</strong> ${e.message}</div>`;
        benchStatusEl.className = 'status error';
        console.error("Benchmark error:", e);
    } finally {
        runBenchButton.disabled = false;
        runFullBenchButton.disabled = false;
    }
}

async function runFullBenchmark() {
    const benchStatusEl = document.getElementById('benchmarkStatus');
    const runBenchButton = document.getElementById('runBenchmarkButton');
    const runFullBenchButton = document.getElementById('runFullBenchmarkButton');

    benchStatusEl.innerHTML = '<div class="spinner"></div>Running full benchmark suite...';
    benchStatusEl.className = 'status loading';
    runBenchButton.disabled = true;
    runFullBenchButton.disabled = true;

    try {
        appState.benchmarkResults = [];
        document.getElementById('benchmarkResults').innerHTML = '';

        const sizes = [10000, 100000, 1000000, 5000000, 10000000, 20000000];
        const sortOrders = [0, 1, 2];
        const strategies = [0, 1];

        let totalTests = sizes.length * sortOrders.length * strategies.length;
        let completedTests = 0;

        for (const size of sizes) {
            for (const sortOrder of sortOrders) {
                for (const strategy of strategies) {
                    const threads = strategy === 0 ? 1 : 4;
                    const processingMode = await ProcessingModeManager.determineProcessingMode();

                    benchStatusEl.innerHTML = `<div class="spinner"></div>Running benchmark ${completedTests + 1}/${totalTests}: ${size.toLocaleString()} items, ${strategy === 0 ? 'Blocking' : 'Parallel'}, ${sortOrder === 0 ? 'Natural' : sortOrder === 1 ? 'Short-to-Long' : 'Long-to-Short'} (${processingMode.toUpperCase()})`;

                    await new Promise(resolve => setTimeout(resolve, 50));

                    const startTime = performance.now();
                    let result;

                    if (processingMode === 'wasm' && appState.wasmModule) {
                        result = WASMClient.runBenchmark(size, sortOrder, strategy, threads);
                        result.processingMode = 'WASM';
                    } else {
                        const config = {
                            itemCount: size,
                            sortOrder: ['NATURAL', 'SHORT_TO_LONG', 'LONG_TO_SHORT'][sortOrder],
                            strategyType: strategy === 0 ? 'BLOCKING' : 'PARALLEL',
                            threadCount: threads
                        };
                        result = await APIClient.runBenchmark(config);
                        result.processingMode = 'API';
                    }

                    const jsTime = performance.now() - startTime;
                    result.jsProcessingTime = jsTime;

                    addBenchmarkResult(result);
                    completedTests++;
                }
            }
        }

        benchStatusEl.textContent = `✅ Full benchmark suite completed (${totalTests} tests)`;
        benchStatusEl.className = 'status success';

    } catch (e) {
        benchStatusEl.innerHTML = `<div class="error">❌ <strong>Error:</strong> ${e.message}</div>`;
        benchStatusEl.className = 'status error';
        console.error("Benchmark error:", e);
    } finally {
        runBenchButton.disabled = false;
        runFullBenchButton.disabled = false;
    }
}

function addBenchmarkResult(result) {
    appState.benchmarkResults.push(result);

    const tbody = document.getElementById('benchmarkResults');
    const row = document.createElement('tr');

    // Handle different result formats (WASM vs API)
    const size = result.size || result.itemCount;
    const strategy = result.strategy || result.strategyUsed;
    const sortOrder = result.order || result.sortOrder;
    const sortingTime = result.sortingTime || result.sortingTimeMs;
    const packingTime = result.packingTime || result.packingTimeMs;
    const totalTime = result.totalTime || result.totalTimeMs;
    const totalPacks = result.totalPacks || result.packCount;
    const utilization = result.utilizationPercent;
    const threadCount = result.numThreads || result.threadCount;

    row.innerHTML = `
        <td>${size.toLocaleString()}</td>
        <td>${strategy}</td>
        <td>${strategy.includes('PARALLEL') || strategy.includes('Parallel') ? threadCount : '-'}</td>
        <td>${sortOrder}</td>
        <td>${sortingTime.toFixed(3)}</td>
        <td>${packingTime.toFixed(3)}</td>
        <td>${totalTime.toFixed(3)}</td>
        <td>${Math.round(size / (totalTime / 1000)).toLocaleString()}</td>
        <td>${totalPacks.toLocaleString()}</td>
        <td>${utilization.toFixed(1)}%</td>
        <td>${result.processingMode}</td>
    `;

    tbody.appendChild(row);
}

function exportBenchmarkResults() {
    if (appState.benchmarkResults.length === 0) {
        alert("No results to export!");
        return;
    }

    let csv = "Size,Strategy,Threads,Sort Order,Sort Time (ms),Pack Time (ms),Total Time (ms),Items/sec,Packs,Utilization %,Processing Mode\n";
    appState.benchmarkResults.forEach(res => {
        const size = res.size || res.itemCount;
        const strategy = res.strategy || res.strategyUsed;
        const sortOrder = res.order || res.sortOrder;
        const sortingTime = res.sortingTime || res.sortingTimeMs;
        const packingTime = res.packingTime || res.packingTimeMs;
        const totalTime = res.totalTime || res.totalTimeMs;
        const totalPacks = res.totalPacks || res.packCount;
        const utilization = res.utilizationPercent;
        const threadCount = res.numThreads || res.threadCount;

        csv += `${size},${strategy},${strategy.includes('PARALLEL') || strategy.includes('Parallel') ? threadCount : "-"},${sortOrder},`;
        csv += `${sortingTime.toFixed(3)},${packingTime.toFixed(3)},${totalTime.toFixed(3)},`;
        csv += `${Math.round(size / (totalTime / 1000))},${totalPacks},${utilization.toFixed(1)},${res.processingMode}\n`;
    });

    const blob = new Blob([csv], { type: 'text/csv' });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = `pack_benchmark_${new Date().toISOString().slice(0, 10)}.csv`;
    document.body.appendChild(a);
    a.click();
    document.body.removeChild(a);
    URL.revokeObjectURL(url);
}

// Tab Management
function switchTab(tabName) {
    if (tabName === 'packing') {
        document.getElementById('packingSection').style.display = 'flex';
        document.getElementById('benchmarkSection').style.display = 'none';
        document.getElementById('packingTab').classList.add('active');
        document.getElementById('benchmarkTab').classList.remove('active');
    } else {
        document.getElementById('packingSection').style.display = 'none';
        document.getElementById('benchmarkSection').style.display = 'block';
        document.getElementById('packingTab').classList.remove('active');
        document.getElementById('benchmarkTab').classList.add('active');
    }
}

// Application Initialization
async function initializeApplication() {
    updateStatus('Initializing system...', 'loading');
    ProcessingModeManager.updateCurrentModeDisplay('detecting');

    // Initialize system profiler
    await systemProfiler.initialize();

    // Try to load WASM module
    const wasmLoaded = await WASMLoader.loadModule();
    
    if (wasmLoaded) {
        // Run CPU/Memory tests now that WASM is loaded
        systemProfiler.runCPUMemoryTest();
    }

    // Test API connection if needed
    const selectedMode = document.querySelector('input[name="processingMode"]:checked').value;
    if (selectedMode === 'api' || selectedMode === 'auto') {
        await APIClient.testConnection();
    }

    // Determine initial processing mode
    const initialMode = await ProcessingModeManager.determineProcessingMode();
    ProcessingModeManager.updateCurrentModeDisplay(initialMode);

    // Enable run button
    document.getElementById('runButton').disabled = false;
    updateStatus('✅ System initialized successfully!', 'success');
}

// Event Listeners Setup
function setupEventListeners() {
    // Processing mode change handlers
    document.querySelectorAll('input[name="processingMode"]').forEach(radio => {
        radio.addEventListener('change', async (e) => {
            ProcessingModeManager.updateCurrentModeDisplay('detecting');
            const mode = await ProcessingModeManager.determineProcessingMode();
            ProcessingModeManager.updateCurrentModeDisplay(mode);
            
            if (mode === 'api') {
                await APIClient.testConnection();
            }
        });
    });

    // Main functionality
    document.getElementById('runButton').addEventListener('click', runPacking);
    document.getElementById('generateButton').addEventListener('click', generateTestData);
    document.getElementById('downloadInputButton').addEventListener('click', downloadInputData);

    // API controls
    document.getElementById('testConnectionButton')?.addEventListener('click', APIClient.testConnection);
    document.getElementById('apiBaseUrl')?.addEventListener('change', APIClient.testConnection);
    document.getElementById('cancelJobButton')?.addEventListener('click', cancelCurrentJob);

    // Tab controls
    document.getElementById('packingTab').addEventListener('click', () => switchTab('packing'));
    document.getElementById('benchmarkTab').addEventListener('click', () => switchTab('benchmark'));

    // Benchmark controls
    document.getElementById('runBenchmarkButton').addEventListener('click', runSingleBenchmark);
    document.getElementById('runFullBenchmarkButton').addEventListener('click', runFullBenchmark);
    document.getElementById('exportBenchmarkButton').addEventListener('click', exportBenchmarkResults);

    // Initialize with packing tab active
    switchTab('packing');
}

// Application Entry Point
document.addEventListener('DOMContentLoaded', async () => {
    setupEventListeners();
    await initializeApplication();
});

// Export for global access (if needed)
window.appState = appState;
window.systemProfiler = systemProfiler;
