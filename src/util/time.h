#ifndef DINARI_UTIL_TIME_H
#define DINARI_UTIL_TIME_H

#include "dinari/types.h"
#include <string>

namespace dinari {

/**
 * @brief Time utilities for Dinari blockchain
 *
 * Provides time-related functions for timestamps, time formatting,
 * and time-based calculations.
 */

class Time {
public:
    /**
     * @brief Get current Unix timestamp (seconds since epoch)
     * @return Current timestamp
     */
    static Timestamp GetCurrentTime();

    /**
     * @brief Get current Unix timestamp in milliseconds
     * @return Current timestamp in milliseconds
     */
    static uint64_t GetCurrentTimeMillis();

    /**
     * @brief Get current Unix timestamp in microseconds
     * @return Current timestamp in microseconds
     */
    static uint64_t GetCurrentTimeMicros();

    /**
     * @brief Format timestamp to human-readable string
     * @param timestamp Unix timestamp
     * @return Formatted string (YYYY-MM-DD HH:MM:SS)
     */
    static std::string FormatTimestamp(Timestamp timestamp);

    /**
     * @brief Format timestamp to ISO 8601 string
     * @param timestamp Unix timestamp
     * @return ISO 8601 formatted string
     */
    static std::string FormatISO8601(Timestamp timestamp);

    /**
     * @brief Parse ISO 8601 string to timestamp
     * @param iso8601 ISO 8601 formatted string
     * @return Unix timestamp
     */
    static Timestamp ParseISO8601(const std::string& iso8601);

    /**
     * @brief Sleep for specified milliseconds
     * @param milliseconds Time to sleep
     */
    static void SleepMillis(uint64_t milliseconds);

    /**
     * @brief Get monotonic time (for measuring intervals)
     * Not affected by system time changes
     * @return Monotonic time in microseconds
     */
    static uint64_t GetMonotonicMicros();

    /**
     * @brief Check if timestamp is in the future
     * @param timestamp Timestamp to check
     * @param tolerance Tolerance in seconds
     * @return true if timestamp is in the future
     */
    static bool IsInFuture(Timestamp timestamp, uint32_t tolerance = 0);

    /**
     * @brief Check if timestamp is in the past
     * @param timestamp Timestamp to check
     * @param tolerance Tolerance in seconds
     * @return true if timestamp is in the past
     */
    static bool IsInPast(Timestamp timestamp, uint32_t tolerance = 0);

    /**
     * @brief Get time difference in seconds
     * @param timestamp1 First timestamp
     * @param timestamp2 Second timestamp
     * @return Difference in seconds (absolute value)
     */
    static int64_t GetDifference(Timestamp timestamp1, Timestamp timestamp2);
};

/**
 * @brief Timer class for measuring elapsed time
 */
class Timer {
public:
    Timer();

    /**
     * @brief Start/restart the timer
     */
    void Start();

    /**
     * @brief Stop the timer and return elapsed time
     * @return Elapsed time in microseconds
     */
    uint64_t Stop();

    /**
     * @brief Get elapsed time without stopping
     * @return Elapsed time in microseconds
     */
    uint64_t Elapsed() const;

    /**
     * @brief Get elapsed time in milliseconds
     * @return Elapsed time in milliseconds
     */
    uint64_t ElapsedMillis() const;

    /**
     * @brief Get elapsed time in seconds
     * @return Elapsed time in seconds
     */
    double ElapsedSeconds() const;

private:
    uint64_t startTime;
    bool running;
};

} // namespace dinari

#endif // DINARI_UTIL_TIME_H
