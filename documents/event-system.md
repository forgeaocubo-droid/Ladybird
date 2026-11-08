# Event System

## Purpose and scope

The event system interprets protocol messages according to the shared event
catalog and maps recognized incoming events to actions supported by the
receiving system. This document focuses on event definitions, validation,
request/response behavior, and action dispatch; the systems that exchange
messages are not relevant to these rules.

The binary message format is defined in
[`communication-protocol.md`](./communication-protocol.md). The protocol
library decodes bytes into a payload; the event library then applies the
catalog and the receiving system's action dictionary.

## Event definitions

Each event definition associates a unique event ID with its type and
processing behavior. A definition can include:

| Field | Purpose |
| --- | --- |
| `id` | Unique unsigned event identifier from 0 to 255 |
| `type` | `BOOL`, `FLOAT32`, or `BYTES` |
| `transmission` | `event`, `analog`, `command`, or `response` |
| `response` | Optional response event; defaults to `ACK` for an event requiring an acknowledgement |
| `info`, `info_type`, `info_value` | Optional data to include in an `INFO` response |
| `min`, `max` | Optional accepted range for incoming floating-point values |

Event IDs and types are shared protocol definitions. A local action dictionary
is separate: it determines which recognized events the particular receiving
system can handle and what action should be dispatched for each one.

The catalog requires `ACK`, `INFO`, and `ERR` response definitions, conventionally
with IDs `0xF0`, `0xF1`, and `0xF2` and types `BOOL`, `BYTES`, and `BOOL`.
The event type is resolved from the catalog; it is not a separate field in
the protocol payload.

## Event message categories

- **Outgoing event/request:** An event or command sent to ask the receiving
  system to process a recognized event. It has a message ID that can be used
  to match the response. Analog and discrete events are also outgoing
  messages; whether an acknowledgement is requested is determined by the
  event's configured response behavior.
- **Incoming request/event:** A valid, non-response event received for
  processing. The receiver validates it, checks its action dictionary, and
  dispatches the associated action when one exists.
- **Response:** `ACK`, `INFO`, or `ERR` sent in answer to a previous request,
  or received as a reply. A response echoes the message ID of the request.
  A received response is matched against an outstanding request; it is not
  treated as a new action request.

This distinction is based on the message's event definition and request
correlation, not on which system or device sent it.

## Incoming event analysis

The event library processes an incoming message in these stages:

1. Receive a decoded payload from the protocol library.
2. Validate that its event ID exists in the shared event catalog.
3. Check that the value length and representation match the event type:
   `BOOL` is one byte containing 0 or 1, `FLOAT32` is a finite four-byte
   value, and `BYTES` is at most 18 bytes.
4. Apply any event-specific constraints, such as configured numeric bounds.
5. If the event is a response, match it to the outstanding message ID and
   process it as a reply.
6. If it is a request/event, look up its event ID in the receiving system's
   action dictionary and return or dispatch the corresponding action.
7. Generate the configured response, when a response is required.

Malformed messages and invalid values fail validation and must not be
dispatched as actions. An event ID that is not present in the protocol catalog
is an unrecognized event and receives the configured `ERR` response when a
response can safely be formed. Do not confuse an unknown protocol event with
a known event that simply has no action in the receiving system.

## Receiving-system action dictionary

Each receiving system must provide its own action dictionary. The dictionary
maps protocol event IDs (or their catalog names) to the actions that system
can execute. For example:

```yaml
actions:
  BUTTON_CONFIRM_PRESSED: confirm_selection
  DEVICE_INFO_REQUEST: provide_information
```

The dictionary is the authority for whether a valid, known event can be
handled by that system. A protocol event may be defined globally yet have no
entry in a particular system's dictionary. In that case, do not dispatch an
action and respond with `ACK` carrying boolean false. If the event has a
corresponding action and is accepted for dispatch, respond with `ACK` carrying
boolean true. This acknowledgement reports whether the event is recognized
and mapped for handling; it does not define the eventual result of performing
the action.

## Response behavior

The response library behavior is:

| Situation | Response |
| --- | --- |
| Known event with an associated action accepted for dispatch | `ACK` with value `true` |
| Known event with no action associated in this system | `ACK` with value `false` |
| Event ID not defined in the protocol catalog | `ERR` indicating an unrecognized event |
| Valid request configured to return information | `INFO` with the configured information |
| Invalid message structure or invalid event value | Reject; do not dispatch an action or produce a success ACK |

Every response echoes the request's message ID. `ACK` uses a one-byte boolean
value. `INFO` carries the configured information using its configured type,
and `ERR` represents a protocol-level event recognition error. The exact
error value encoding must follow the `ERR` event definition.

An incoming response is accepted only if its message ID matches an
outstanding request. The response is consumed once matched; a duplicate
response is not a second reply to that request.

## Event library functions

The event manipulation library provides the following operations:

### Validate an incoming message

Validate the decoded payload against the protocol catalog and report
validation errors explicitly. Checks include whether the event exists, its
value length and type, and any configured constraints. Invalid data must not
produce a success-shaped result or be sent to an action.

### Generate a success acknowledgement

Create a valid `ACK` response with boolean `true` for a known event that has
an associated action accepted for dispatch. Copy the request message ID into
the response.

### Generate an error response

Create a valid `ERR` response for an event ID that is not recognized in the
protocol catalog, using the request message ID. This is distinct from a
recognized event for which this system has no action; that case returns
`ACK(false)`.

### Generate an information response

Create a valid `INFO` response containing the information configured for the
requesting event, using its configured value type and the request message ID.

### Resolve an event to a system action

Accept an incoming event message, validate it, consult the receiving system's
action dictionary, and return or dispatch the corresponding action. If the
event is known but absent from the dictionary, report that no action is
available and generate `ACK(false)`. If a mapped action is accepted for
dispatch, generate `ACK(true)`. The system-specific dictionary must be
provided separately from the shared protocol event catalog.

These operations use the protocol library's encoder to serialize the
responses. Message framing, byte serialization, and event meaning therefore
remain separate responsibilities.

## Example flows

### Known request with an action

1. A message arrives with a valid payload and an event ID defined in the
   protocol catalog.
2. The event library validates its value and finds an associated action in
   the receiving system's action dictionary.
3. The action is accepted for dispatch.
4. If an acknowledgement is requested, the library generates `ACK(true)`
   using the request's message ID.

### Known event without an action

1. A message arrives with a valid event ID and value, both defined by the
   protocol catalog.
2. The event library finds no action for it in the receiving system's
   dictionary and does not dispatch an action.
3. The library generates `ACK(false)` using the request's message ID.

### Unknown event

1. A message arrives with an event ID not present in the protocol catalog.
2. The event library does not treat it as a known event without a mapping.
3. When a valid response can be formed, it generates `ERR` using the
   request's message ID.

### Information request

1. A valid request corresponds to an event configured to return information.
2. The event library resolves the configured information and value type.
3. It generates an `INFO` response that echoes the request's message ID.

## Source of truth

The event catalog is defined in `SDFiles/events.yaml`. The current event
handling implementation is in `src/lib/event_handler/EventHandler.cpp` and
`src/lib/event_handler/EventHandler.hpp`; protocol message serialization is
defined in `src/lib/protocol/ProtocolHandler.cpp`.
