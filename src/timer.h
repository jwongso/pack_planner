#ifndef TIMER_H
#define TIMER_H

#include <chrono>
#include <iostream>
#include <iomanip>

namespace utils {

// High-resolution timer class for timing operations
class timer {
private:
    std::chrono::high_resolution_clock::time_point m_start;
    std::chrono::high_resolution_clock::time_point m_end;
    bool m_running;

public:
    timer() : m_running(false) {}

    void start() {
        m_start = std::chrono::high_resolution_clock::now();
        m_running = true;
    }

    void stop() {
        m_end = std::chrono::high_resolution_clock::now();
        m_running = false;
    }

    long long get_microseconds() const {
        if (m_running) {
            auto now = std::chrono::high_resolution_clock::now();
            return std::chrono::duration_cast<std::chrono::microseconds>(now - m_start).count();
        }
        return std::chrono::duration_cast<std::chrono::microseconds>(m_end - m_start).count();
    }

    long long get_nanoseconds() const {
        if (m_running) {
            auto now = std::chrono::high_resolution_clock::now();
            return std::chrono::duration_cast<std::chrono::nanoseconds>(now - m_start).count();
        }
        return std::chrono::duration_cast<std::chrono::nanoseconds>(m_end - m_start).count();
    }

    double get_milliseconds() const {
        if (m_running) {
            auto now = std::chrono::high_resolution_clock::now();
            return std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(now - m_start).count();
        }
        return std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(m_end - m_start).count();
    }

    // Print timing result with label
    void print_time(const std::string& operation) const {
        std::cout << operation << ": " << std::fixed << std::setprecision(3)
                  << get_milliseconds() << " ms (" << get_microseconds() << " μs)" << std::endl;
    }
};

} // utils

#endif // TIMER_H
