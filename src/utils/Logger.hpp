#pragma once

#include <Arduino.h>

#include "includes/Config.hpp"

namespace chimera {

/** Serial logger using String messages and compile-time default filtering. */
class Logger {
 public:
  /** Creates a logger using project defaults. */
  Logger();
  /** Enables or disables all output. @param enabled Whether messages are
   * emitted. */
  void setEnabled(bool enabled);
  /** Sets the minimum emitted level. @param level DEBUG, INFO, or ERROR. */
  void setLevel(LogLevel level);
  /** Emits a debug message when enabled and permitted. @param message Text to
   * log. */
  void logDebug(const String& message) const;
  /** Emits an info message when enabled and permitted. @param message Text to
   * log. */
  void logInfo(const String& message) const;
  /** Emits an error message when enabled and permitted. @param message Text to
   * log. */
  void logError(const String& message) const;

 private:
  void write(LogLevel level, const char* label, const String& message) const;
  bool enabled_;
  LogLevel level_;
};

extern Logger gLogger;

}  // namespace chimera