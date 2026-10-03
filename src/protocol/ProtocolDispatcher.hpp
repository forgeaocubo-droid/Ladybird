#pragma once

#include <stddef.h>
#include <stdint.h>

#include "includes/ProtocolTypes.hpp"

namespace chimera {

using EventValidator = bool (*)(const ProtocolFrame& frame);
using EventHandler = bool (*)(const ProtocolFrame& frame);

/** Compile-time application contract for one supported event. */
struct EventDefinition {
  uint8_t event;
  ValueType type;
  EventValidator validator;
  EventHandler handler;
};

enum class DispatchResult : uint8_t {
  Accepted,
  UnknownEvent,
  TypeMismatch,
  InvalidValue,
  HandlerRejected,
};

/** Validates frames against an application-owned event catalog. */
class ProtocolDispatcher {
 public:
  /** Creates a dispatcher over a static catalog. @param definitions Catalog
   * entries, or null when no events are approved. @param count Number of
   * entries. */
  ProtocolDispatcher(const EventDefinition* definitions, size_t count);
  /** Validates and routes a frame. Definitions without handlers are rejected.
   * @param frame CRC-validated frame. @return Semantic dispatch result. */
  DispatchResult dispatch(const ProtocolFrame& frame) const;

 private:
  const EventDefinition* definitions_;
  size_t count_;
};

}  // namespace chimera