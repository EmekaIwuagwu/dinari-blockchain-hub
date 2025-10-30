#include "logger.h"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <ctime>

namespace dinari {

Logger& Logger::Instance() {
    static Logger instance;
    return instance;
}

Logger::Logger()
    : currentLevel(LogLevel::INFO)
    , consoleEnabled(true)
    , fileEnabled(true)
{
}

Logger::~Logger() {
    Close();
}

void Logger::Initialize(const std::string& logFilePath, LogLevel level) {
    std::lock_guard<std::mutex> lock(mutex);

    currentLevel = level;

    if (logFile.is_open()) {
        logFile.close();
    }

    logFile.open(logFilePath, std::ios::out | std::ios::app);
    if (!logFile.is_open()) {
        std::cerr << "Failed to open log file: " << logFilePath << std::endl;
        fileEnabled = false;
    } else {
        fileEnabled = true;
        logFile << "\n=== Dinari Blockchain Log Started at " << GetTimestamp() << " ===\n" << std::endl;
    }
}

void Logger::SetLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(mutex);
    currentLevel = level;
}

void Logger::Log(LogLevel level, const std::string& category, const std::string& message) {
    if (level < currentLevel) {
        return;  // Don't log messages below current level
    }

    std::lock_guard<std::mutex> lock(mutex);

    std::string formatted = FormatMessage(level, category, message);

    // Console output
    if (consoleEnabled) {
        if (level >= LogLevel::ERROR) {
            std::cerr << formatted << std::endl;
        } else {
            std::cout << formatted << std::endl;
        }
    }

    // File output
    if (fileEnabled && logFile.is_open()) {
        logFile << formatted << std::endl;
    }
}

void Logger::Trace(const std::string& category, const std::string& message) {
    Log(LogLevel::TRACE, category, message);
}

void Logger::Debug(const std::string& category, const std::string& message) {
    Log(LogLevel::DEBUG, category, message);
}

void Logger::Info(const std::string& category, const std::string& message) {
    Log(LogLevel::INFO, category, message);
}

void Logger::Warning(const std::string& category, const std::string& message) {
    Log(LogLevel::WARNING, category, message);
}

void Logger::Error(const std::string& category, const std::string& message) {
    Log(LogLevel::ERROR, category, message);
}

void Logger::Fatal(const std::string& category, const std::string& message) {
    Log(LogLevel::FATAL, category, message);
}

void Logger::Flush() {
    std::lock_guard<std::mutex> lock(mutex);
    if (logFile.is_open()) {
        logFile.flush();
    }
}

void Logger::Close() {
    std::lock_guard<std::mutex> lock(mutex);
    if (logFile.is_open()) {
        logFile << "\n=== Dinari Blockchain Log Closed at " << GetTimestamp() << " ===\n" << std::endl;
        logFile.close();
    }
}

std::string Logger::FormatMessage(LogLevel level, const std::string& category, const std::string& message) {
    std::ostringstream oss;
    oss << "[" << GetTimestamp() << "] "
        << "[" << LevelToString(level) << "] "
        << "[" << category << "] "
        << message;
    return oss.str();
}

std::string Logger::GetTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;

    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    oss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return oss.str();
}

std::string Logger::LevelToString(LogLevel level) {
    switch (level) {
        case LogLevel::TRACE:   return "TRACE";
        case LogLevel::DEBUG:   return "DEBUG";
        case LogLevel::INFO:    return "INFO ";
        case LogLevel::WARNING: return "WARN ";
        case LogLevel::ERROR:   return "ERROR";
        case LogLevel::FATAL:   return "FATAL";
        default:                return "UNKNOWN";
    }
}

} // namespace dinari
