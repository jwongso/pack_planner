#include "Timer.h"

Timer::Timer() : isRunning_(false) {
}

void Timer::start() {
    startTime_ = std::chrono::high_resolution_clock::now();
    isRunning_ = true;
}

double Timer::stop() {
    if (!isRunning_) return 0.0;
    
    endTime_ = std::chrono::high_resolution_clock::now();
    isRunning_ = false;
    
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime_ - startTime_);
    return duration.count() / 1000.0; // Convert to milliseconds
}

double Timer::elapsed() const {
    if (!isRunning_) {
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime_ - startTime_);
        return duration.count() / 1000.0; // Convert to milliseconds
    }
    
    auto currentTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(currentTime - startTime_);
    return duration.count() / 1000.0; // Convert to milliseconds
}

double Timer::elapsedMicroseconds() const {
    if (!isRunning_) {
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime_ - startTime_);
        return static_cast<double>(duration.count());
    }
    
    auto currentTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(currentTime - startTime_);
    return static_cast<double>(duration.count());
}

void Timer::reset() {
    isRunning_ = false;
}
