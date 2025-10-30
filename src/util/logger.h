#ifndef DINARI_UTIL_LOGGER_H
#define DINARI_UTIL_LOGGER_H

#include <string>
#include <memory>
#include <mutex>
#include <fstream>
#include <sstream>

namespace dinari {

/**
 * @brief Logging system for Dinari blockchain
 *
 * Thread-safe logging with multiple severity levels.
 * Logs to both console and file.
 */

enum class LogLevel {
    TRACE,
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    FATAL
};

class Logger {
public:
    static Logger& Instance();

    // Initialize logger
    void Initialize(const std::string& logFile, LogLevel level = LogLevel::INFO);

    // Set log level
    void SetLevel(LogLevel level);

    // Get log level
    LogLevel GetLevel() const { return currentLevel; }

    // Log functions
    void Log(LogLevel level, const std::string& category, const std::string& message);
    void Trace(const std::string& category, const std::string& message);
    void Debug(const std::string& category, const std::string& message);
    void Info(const std::string& category, const std::string& message);
    void Warning(const std::string& category, const std::string& message);
    void Error(const std::string& category, const std::string& message);
    void Fatal(const std::string& category, const std::string& message);

    // Enable/disable console output
    void SetConsoleOutput(bool enabled) { consoleEnabled = enabled; }

    // Enable/disable file output
    void SetFileOutput(bool enabled) { fileEnabled = enabled; }

    // Flush logs to disk
    void Flush();

    // Close logger
    void Close();

private:
    Logger();
    ~Logger();

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    std::string FormatMessage(LogLevel level, const std::string& category, const std::string& message);
    std::string GetTimestamp();
    std::string LevelToString(LogLevel level);

    LogLevel currentLevel;
    bool consoleEnabled;
    bool fileEnabled;
    std::ofstream logFile;
    std::mutex mutex;
};

// Convenience macros
#define LOG_TRACE(category, message) dinari::Logger::Instance().Trace(category, message)
#define LOG_DEBUG(category, message) dinari::Logger::Instance().Debug(category, message)
#define LOG_INFO(category, message) dinari::Logger::Instance().Info(category, message)
#define LOG_WARNING(category, message) dinari::Logger::Instance().Warning(category, message)
#define LOG_ERROR(category, message) dinari::Logger::Instance().Error(category, message)
#define LOG_FATAL(category, message) dinari::Logger::Instance().Fatal(category, message)

// Stream-style logging
class LogStream {
public:
    LogStream(LogLevel level, const std::string& category)
        : level(level), category(category) {}

    ~LogStream() {
        Logger::Instance().Log(level, category, stream.str());
    }

    template<typename T>
    LogStream& operator<<(const T& value) {
        stream << value;
        return *this;
    }

private:
    LogLevel level;
    std::string category;
    std::stringstream stream;
};

// Stream-style macros
#define LOG_STREAM(level, category) dinari::LogStream(level, category)
#define LOG_TRACE_STREAM(category) dinari::LogStream(dinari::LogLevel::TRACE, category)
#define LOG_DEBUG_STREAM(category) dinari::LogStream(dinari::LogLevel::DEBUG, category)
#define LOG_INFO_STREAM(category) dinari::LogStream(dinari::LogLevel::INFO, category)
#define LOG_WARNING_STREAM(category) dinari::LogStream(dinari::LogLevel::WARNING, category)
#define LOG_ERROR_STREAM(category) dinari::LogStream(dinari::LogLevel::ERROR, category)

} // namespace dinari

#endif // DINARI_UTIL_LOGGER_H
