#pragma once

#include <chrono>

class Timer {
public:
    Timer();
    
    // Start timing
    void start();
    
    // Stop timing and return elapsed time
    double stop();  // Returns milliseconds
    
    // Get elapsed time without stopping
    double elapsed() const;  // Returns milliseconds
    
    // Get elapsed time in microseconds
    double elapsedMicroseconds() const;
    
    // Reset timer
    void reset();

private:
    std::chrono::high_resolution_clock::time_point startTime_;
    std::chrono::high_resolution_clock::time_point endTime_;
    bool isRunning_;
};
