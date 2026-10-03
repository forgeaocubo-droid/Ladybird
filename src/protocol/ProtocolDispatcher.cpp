#include "protocol/ProtocolDispatcher.hpp"

namespace chimera {

ProtocolDispatcher::ProtocolDispatcher(const EventDefinition* definitions,
                                       size_t count)
    : definitions_(definitions), count_(definitions == nullptr ? 0 : count) {}

DispatchResult ProtocolDispatcher::dispatch(const ProtocolFrame& frame) const {
  for (size_t index = 0; index < count_; ++index) {
    const EventDefinition& definition = definitions_[index];
    if (definition.event != frame.event) continue;
    if (definition.type != frame.type) return DispatchResult::TypeMismatch;
    if (definition.validator != nullptr && !definition.validator(frame))
      return DispatchResult::InvalidValue;
    if (definition.handler == nullptr) return DispatchResult::InvalidValue;
    return definition.handler(frame) ? DispatchResult::Accepted
                                    : DispatchResult::HandlerRejected;
  }
  return DispatchResult::UnknownEvent;
}

}  // namespace chimera