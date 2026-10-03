#pragma once

#include <stddef.h>
#include <stdint.h>

#include "protocol/ProtocolDispatcher.hpp"

namespace chimera {

extern const EventDefinition kEventCatalog[];
extern const size_t kEventCatalogCount;

const EventDefinition* eventCatalog(size_t* count = nullptr);
const char* eventName(uint8_t eventId);
uint16_t eventThreshold(uint8_t eventId);
bool isEventActionConfigured(uint8_t eventId);
void setInfoLedLatched(bool latched);
bool infoLedLatched();
void setInfoLedActivityActive(bool active);
bool infoLedActivityActive();

}  // namespace chimera
