#include "time.h"
#include <chrono>
#include <thread>
#include <iomanip>
#include <sstream>
#include <ctime>

namespace dinari {

Timestamp Time::GetCurrentTime() {
    return static_cast<Timestamp>(
        std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count()
    );
}

uint64_t Time::GetCurrentTimeMillis() {
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count()
    );
}

uint64_t Time::GetCurrentTimeMicros() {
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count()
    );
}

std::string Time::FormatTimestamp(Timestamp timestamp) {
    std::time_t time = static_cast<std::time_t>(timestamp);
    std::tm* tm = std::localtime(&time);

    std::ostringstream oss;
    oss << std::put_time(tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

std::string Time::FormatISO8601(Timestamp timestamp) {
    std::time_t time = static_cast<std::time_t>(timestamp);
    std::tm* tm = std::gmtime(&time);

    std::ostringstream oss;
    oss << std::put_time(tm, "%Y-%m-%dT%H:%M:%SZ");
    return oss.str();
}

Timestamp Time::ParseISO8601(const std::string& iso8601) {
    std::tm tm = {};
    std::istringstream ss(iso8601);
    ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");

    if (ss.fail()) {
        return 0;
    }

    return static_cast<Timestamp>(std::mktime(&tm));
}

void Time::SleepMillis(uint64_t milliseconds) {
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

uint64_t Time::GetMonotonicMicros() {
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now().time_since_epoch()
        ).count()
    );
}

bool Time::IsInFuture(Timestamp timestamp, uint32_t tolerance) {
    Timestamp now = GetCurrentTime();
    return timestamp > (now + tolerance);
}

bool Time::IsInPast(Timestamp timestamp, uint32_t tolerance) {
    Timestamp now = GetCurrentTime();
    return timestamp < (now - tolerance);
}

int64_t Time::GetDifference(Timestamp timestamp1, Timestamp timestamp2) {
    int64_t diff = static_cast<int64_t>(timestamp1) - static_cast<int64_t>(timestamp2);
    return diff < 0 ? -diff : diff;
}

// Timer implementation

Timer::Timer() : startTime(0), running(false) {
    Start();
}

void Timer::Start() {
    startTime = Time::GetMonotonicMicros();
    running = true;
}

uint64_t Timer::Stop() {
    if (!running) {
        return 0;
    }

    uint64_t elapsed = Time::GetMonotonicMicros() - startTime;
    running = false;
    return elapsed;
}

uint64_t Timer::Elapsed() const {
    if (!running) {
        return 0;
    }

    return Time::GetMonotonicMicros() - startTime;
}

uint64_t Timer::ElapsedMillis() const {
    return Elapsed() / 1000;
}

double Timer::ElapsedSeconds() const {
    return static_cast<double>(Elapsed()) / 1000000.0;
}

} // namespace dinari
