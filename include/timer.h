#pragma once

#include <chrono>

/**
 * @brief Utility class for measuring execution time
 */
class timer {
public:
    /**
     * @brief Construct a new timer object
     */
    timer() noexcept
        : m_is_running(false)
    {}

    /**
     * @brief Start timing
     */
    void start() noexcept {
        m_start_time = std::chrono::high_resolution_clock::now();
        m_is_running = true;
    }

    /**
     * @brief Stop timing and return elapsed time in milliseconds
     * @return double Elapsed time in milliseconds
     */
    [[nodiscard]] double stop() noexcept {
        if (!m_is_running) return 0.0;

        m_end_time = std::chrono::high_resolution_clock::now();
        m_is_running = false;

        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(m_end_time - m_start_time);
        return duration.count() / 1000.0; // Convert to milliseconds
    }

    /**
     * @brief Get elapsed time in milliseconds without stopping
     * @return double Elapsed time in milliseconds
     */
    [[nodiscard]] double elapsed() const noexcept {
        if (!m_is_running) {
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(m_end_time - m_start_time);
            return duration.count() / 1000.0; // Convert to milliseconds
        }

        auto current_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(current_time - m_start_time);
        return duration.count() / 1000.0; // Convert to milliseconds
    }

    /**
     * @brief Get elapsed time in microseconds
     * @return double Elapsed time in microseconds
     */
    [[nodiscard]] double elapsed_microseconds() const noexcept {
        if (!m_is_running) {
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(m_end_time - m_start_time);
            return static_cast<double>(duration.count());
        }

        auto current_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(current_time - m_start_time);
        return static_cast<double>(duration.count());
    }

    /**
     * @brief Reset timer
     */
    void reset() noexcept {
        m_is_running = false;
    }

private:
    std::chrono::high_resolution_clock::time_point m_start_time;
    std::chrono::high_resolution_clock::time_point m_end_time;
    bool m_is_running;
};
