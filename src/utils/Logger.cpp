#include "utils/Logger.hpp"

namespace chimera {

Logger gLogger;

Logger::Logger() : enabled_(kLoggingEnabled), level_(kDefaultLogLevel) {}

void Logger::setEnabled(bool enabled) { enabled_ = enabled; }
void Logger::setLevel(LogLevel level) { level_ = level; }

void Logger::write(LogLevel level, const char* label,
                   const String& message) const {
  if (enabled_ && static_cast<uint8_t>(level) >= static_cast<uint8_t>(level_)) {
    Serial.print('[');
    Serial.print(label);
    Serial.print(F("] "));
    Serial.println(message);
  }
}

void Logger::logDebug(const String& message) const {
  write(LogLevel::Debug, "DEBUG", message);
}
void Logger::logInfo(const String& message) const {
  write(LogLevel::Info, "INFO", message);
}
void Logger::logError(const String& message) const {
  write(LogLevel::Error, "ERROR", message);
}

}  // namespace chimera