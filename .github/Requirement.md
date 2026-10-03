# LadyBird Robot Controller Firmware Requirements

## 1. Purpose and Scope

This document defines the implementation requirements for the LadyBird robot-side
firmware running on an ESP32-C3 Super Mini. The firmware receives and validates
commands from a handheld controller through Wi-Fi or an nRF24L01 radio, sends
protocol responses on the ingress channel, and performs the explicitly defined
application actions.

This is a receiver/robot firmware specification, not a specification for the
handheld controller. The controller's physical buttons, joysticks, and local
input GPIO assignments are outside this firmware's scope. Motor drivers and
other robot actuators are also outside the current scope because no hardware
mapping or action requirements have been supplied.

The firmware shall:

- Operate Wi-Fi Access Point (AP), Wi-Fi Station (STA), and nRF24L01 RF
  communication simultaneously.
- Use the EventProtocol v1.0 frame format and event/type identifiers in
  `protocol.yaml`.
- Validate every complete frame before invoking application behavior.
- Return ACK, ERR, or REPT responses through the channel on which the request
  arrived, as specified below.
- Control the status and information LEDs, run startup health checks, and
  provide operational logs.
- Keep sensor measurement transmission and actuator control extensible without
  inventing unspecified hardware behavior.

The existing drivers and protocol implementation should be extended or corrected
where needed; implementations that already satisfy these requirements must not
be needlessly recreated.

## 2. Normative Language

The words **MUST**, **MUST NOT**, **SHOULD**, and **MAY** express requirement
strength. A MUST-level requirement is mandatory for acceptance. A SHOULD-level
requirement may be deferred only when there is a documented reason.

## 3. Target Platform and Build

| Item | Requirement |
|---|---|
| Board | ESP32-C3 Super Mini |
| PlatformIO board identifier | `esp32-c3-devkitm-1` |
| Framework | Arduino |
| Language | C++11-compatible or later supported by the selected Arduino core |
| Build configuration | PlatformIO project with `platformio.ini` |
| RF library | RF24 library |
| Serial logging | 115200 baud |

The project MUST compile for the specified board using the checked-in PlatformIO
configuration. Configuration values such as credentials, ports, RF settings,
logging level, and timing constants MUST be centralized in `Config.hpp` rather
than duplicated as driver-local literals.

## 4. Required Project Organization

The target source organization is:

```text
project-root/
├── platformio.ini
├── protocol.yaml
└── src/
    ├── main.cpp
    ├── drivers/
    │   ├── OutputPorts.hpp / .cpp
    │   ├── InputPorts.hpp / .cpp
    │   ├── WifiAPSTA.hpp / .cpp
    │   ├── NRF24Radio.hpp / .cpp
    │   └── UltrasonicSensor.hpp / .cpp
    ├── includes/
    │   ├── PinMap.hpp
    │   ├── Config.hpp
    │   └── ProtocolTypes.hpp
    ├── libs/
    │   └── Events.hpp / .cpp
    ├── protocol/
    │   ├── ProtocolCodec.hpp / .cpp
    │   └── ProtocolDispatcher.hpp / .cpp
    └── utils/
        └── Logger.hpp / .cpp
```

Header suffixes already used in the project (`.h` versus `.hpp`) MAY be retained
consistently if changing them would provide no functional benefit. The same
applies to the existing `Logger.hpp` and `ProtocolDispatcher.hpp` names. The
responsibilities and separation of concerns in this layout are mandatory.

All drivers MUST be object-oriented wrappers around the corresponding hardware
or service API. Shared helpers MUST be implemented once in `utils/` or the
appropriate common module rather than duplicated in drivers.

### 4.1 Module Responsibilities

| Module | Responsibility |
|---|---|
| `main.cpp` | Initialize services, run non-blocking main-loop processing, route received bytes, and update health/activity indicators. It MUST NOT contain duplicated protocol or hardware-driver logic. |
| `OutputPorts` | Initialize and control digital outputs, including active-high/active-low LED behavior. |
| `InputPorts` | Reusable debounced digital and analog input abstraction. Local controller inputs are not required by this robot firmware, but the existing reusable driver SHOULD remain functional. |
| `WifiAPSTA` | Start AP and STA concurrently, expose TCP client data and connection events, and transmit response bytes over the correct Wi-Fi interface/client. |
| `NRF24Radio` | Configure and operate the nRF24L01 using the RF24 library, including bounded packet reads/writes and non-blocking operation where supported. |
| `UltrasonicSensor` | Own ultrasonic GPIO access, validate the startup reading, and perform bounded periodic measurements without blocking normal communication processing. |
| `Events` | Define the event catalog, validation metadata, analog thresholds, and configurable event-to-action mapping. |
| `ProtocolCodec` | Incrementally parse byte streams, encode frames, and calculate/verify CRC16-MODBUS. It MUST NOT execute application actions. |
| `ProtocolDispatcher` | Validate decoded event semantics, invoke configured event handlers, and construct appropriate response frames. |
| `Logger` | Provide shared level-filtered logging APIs used by all modules. |
| `PinMap.hpp` | Be the only source of GPIO assignments. |
| `Config.hpp` | Hold compile-time configuration values and timing/logging defaults. |
| `ProtocolTypes.hpp` | Hold shared protocol event/type/frame definitions and protocol constants. |

## 5. Hardware and GPIO Requirements

Every GPIO assignment MUST be declared in `PinMap.hpp`. Drivers and application
code MUST refer to named constants and MUST NOT repeat numeric pin assignments.

| Peripheral | Signal | GPIO |
|---|---|---:|
| nRF24L01 | CSN | 10 |
| nRF24L01 / shared SPI | MOSI | 7 |
| nRF24L01 / shared SPI | SCK | 6 |
| nRF24L01 / shared SPI | MISO | 2 |
| nRF24L01 | CE | 5 |
| nRF24L01 | IRQ | Not connected; firmware does not use it |
| Status LED | Output | 0 |
| Information LED | Output | 1 |
| Ultrasonic sensor | Trigger | 4 |
| Ultrasonic sensor | Echo | 3 |

The status and information LEDs are both output indications and MUST be driven
through the central GPIO map, not hard-coded in individual drivers. The
firmware MUST support the configured AP+STA Wi-Fi mode concurrently by allowing
both the soft AP and station interfaces to remain active in the same runtime.

The RF24 library MUST use the configured CE/CSN pins and the SPI bus pins above.
IRQ MUST NOT be assumed to be connected.

The ultrasonic sensor is currently a health-check/diagnostic peripheral only.
The firmware MUST check that its startup reading is non-zero and MUST log its
reading once per second after initialization. It MUST NOT transmit ultrasonic
measurements or map them to an event until a protocol event and wire format are
approved. Its measurement implementation MUST have a bounded timeout so a
missing or disconnected sensor cannot block communication processing.

No motor or actuator pins are defined. The firmware MUST NOT drive unspecified
pins or infer motor behavior from joystick events.

## 6. Configuration

### 6.1 Wi-Fi

Configuration MUST permit the operator to specify, at compile time:

- AP SSID, password, and TCP port.
- STA SSID, password, and TCP port.
- Any AP/STA address settings required by the selected Arduino Wi-Fi API.

AP and STA MUST be active concurrently. Wi-Fi server/client handling MUST
identify the ingress interface so replies can be sent on that same interface.
The configured ports MUST be logged during startup.

At startup, the AP log MUST include the configured AP SSID, password, and port,
as requested in the source notes. The STA log MUST include the connected
network's SSID, assigned IP address, and port. Since the AP password is exposed
in the serial log, access to that log MUST be treated as access to a credential.

### 6.2 RF

The RF channel, address, address width, power level, and data rate MUST be
configurable in `Config.hpp`. The implementation MUST validate values against
the capabilities of the RF24 library and radio before applying them. The
default payload limit MUST respect the nRF24L01 maximum payload of 32 bytes.

### 6.3 Logging and Timing

Logging enablement, default level, status LED blink period, button debounce
default, Wi-Fi connection timeout, and any service expiry/timeout constants
MUST be centralized in configuration.

When DEBUG logging is enabled, every received payload from an active
communication interface MUST be logged in hexadecimal before it is passed to
the protocol decoder. Debug logs SHOULD identify significant initialization,
transport, measurement, decoding, and event-dispatch operations without
producing an unbounded per-loop log stream.

## 7. EventProtocol v1.0

`protocol.yaml` is the source of truth for event and value-type identifiers.
`ProtocolTypes.hpp` and `Events` MUST agree with it. Changes to event IDs or
types MUST be reflected in both the source catalog and protocol documentation.

### 7.1 Frame Layout

```text
+------+-------+------+----------------+----------+
| SOF  | EVENT | TYPE | VALUE          | CRC16    |
+------+-------+------+----------------+----------+
| 1 B  | 1 B   | 1 B  | N bytes        | 2 B      |
+------+-------+------+----------------+----------+
```

- `SOF` is `0xAA`. It is framing only and is excluded from the CRC input.
- `EVENT` is one byte.
- `TYPE` is one byte and determines fixed `VALUE` length for fixed-size types.
- `VALUE` uses little-endian byte order for multi-byte numeric values.
- `CRC16` is transmitted low byte first and is computed only over
  `EVENT + TYPE + VALUE`.
- CRC MUST be CRC16-MODBUS with reflected polynomial `0xA001`, initial value
  `0xFFFF`, and xor-out `0x0000`.

The codec MUST be incremental and accept frames split across multiple calls or
transport reads. It MUST support independent decoder state for each independent
byte stream/session. A decoder MUST NOT combine bytes from different TCP clients
or from different communication channels.

All received payloads from active communication interfaces MUST pass through
the same protocol decoding and event-validation/dispatch path. Each independent
transport stream MUST retain its own decoder state.

### 7.2 Value Types

| Type | ID | Fixed value size |
|---|---:|---:|
| `BOOL` | `0x01` | 1 byte |
| `UINT8` | `0x02` | 1 byte |
| `INT8` | `0x03` | 1 byte |
| `UINT16` | `0x04` | 2 bytes |
| `INT16` | `0x05` | 2 bytes |
| `UINT32` | `0x06` | 4 bytes |
| `INT32` | `0x07` | 4 bytes |
| `FLOAT32` | `0x08` | 4 bytes |
| `FLOAT64` | `0x09` | 8 bytes |
| `BYTES` | `0x0A` | Variable; length semantics are unresolved (see Section 15) |

Boolean values MUST be encoded as one byte (`0` or `1`). Decoding any other
boolean byte MUST be treated as semantically invalid. Numeric values MUST be
encoded and decoded little-endian. Floating-point values MUST use IEEE-754
representation as supported by the target platform; non-finite values MUST be
rejected during semantic validation.

### 7.3 Receive State Machine

The decoder MUST implement the conceptual states:

```text
IDLE -> RECEIVING_FRAME -> VERIFY_CRC -> PROCESS or SEND_REPT -> IDLE
```

1. In `IDLE`, ignore bytes until `0xAA` is received.
2. After a frame starts, `0xAA` inside event/type/value/CRC positions is data;
   the decoder MUST NOT restart the frame at that byte.
3. After reading `TYPE`, determine the value length from the type table before
   consuming the value and CRC.
4. Unsupported types or impossible lengths MUST terminate the current frame
   without dispatching it. The implementation MUST return to a known idle state
   and log the rejection. Whether such a type can be answered with ERR is
   unresolved because a valid frame boundary cannot always be established.
5. On CRC mismatch, the frame MUST NOT be dispatched and MUST produce a REPT
   response on the ingress channel.
6. On valid CRC and invalid event/type/value/command semantics, the frame MUST
   NOT execute an application action and MUST produce ERR.
7. On valid CRC and valid semantics, the event handler MUST execute (if one is
   configured) and the sender MUST receive ACK.

No command may take effect before the whole frame has passed CRC and semantic
validation.

### 7.4 Event Catalog

The following event IDs, types, fixed values, ranges, and thresholds are
required. A range or threshold applies whenever the event is validated or
generated; event handlers MUST use the catalog rather than duplicate values.

| Event | ID | Type | Value / range | Transmission kind | Threshold |
|---|---:|---|---|---|---:|
| `BUTTON_UP_PRESSED` | `0x01` | BOOL | `true` | Event | — |
| `BUTTON_UP_RELEASED` | `0x02` | BOOL | `false` | Event | — |
| `BUTTON_DOWN_PRESSED` | `0x03` | BOOL | `true` | Event | — |
| `BUTTON_DOWN_RELEASED` | `0x04` | BOOL | `false` | Event | — |
| `BUTTON_LEFT_PRESSED` | `0x05` | BOOL | `true` | Event | — |
| `BUTTON_LEFT_RELEASED` | `0x06` | BOOL | `false` | Event | — |
| `BUTTON_RIGHT_PRESSED` | `0x07` | BOOL | `true` | Event | — |
| `BUTTON_RIGHT_RELEASED` | `0x08` | BOOL | `false` | Event | — |
| `BUTTON_CONFIRM_PRESSED` | `0x09` | BOOL | `true` | Event | — |
| `BUTTON_BACK_PRESSED` | `0x0A` | BOOL | `true` | Event | — |
| `BUTTON_CONFIRM_RELEASED` | `0x0B` | BOOL | `false` | Event | — |
| `BUTTON_BACK_RELEASED` | `0x0C` | BOOL | `false` | Event | — |
| `LEFT_JOY_BUTTON_PRESSED` | `0x0D` | BOOL | `true` | Event | — |
| `LEFT_JOY_BUTTON_RELEASED` | `0x0E` | BOOL | `false` | Event | — |
| `LEFT_JOY_X` | `0x0F` | UINT16 | 0-4095 | Analog | 64 |
| `LEFT_JOY_Y` | `0x10` | UINT16 | 0-4095 | Analog | 64 |
| `RIGHT_JOY_BUTTON_PRESSED` | `0x11` | BOOL | `true` | Event | — |
| `RIGHT_JOY_BUTTON_RELEASED` | `0x12` | BOOL | `false` | Event | — |
| `RIGHT_JOY_X` | `0x13` | UINT16 | 0-4095 | Analog | 64 |
| `RIGHT_JOY_Y` | `0x14` | UINT16 | 0-4095 | Analog | 64 |
| `SENSOR_X` | `0x15` | FLOAT32 | -20 to 20 | Analog | 2 |
| `SENSOR_Y` | `0x16` | FLOAT32 | -20 to 20 | Analog | 2 |
| `SENSOR_Z` | `0x17` | FLOAT32 | -20 to 20 | Analog | 2 |
| `DEVICE_STATUS` | `0x18` | UINT8 | 0-255 | Event | — |
| `LEFT_JOY_X_THRESHOLD_PRESSED` | `0x19` | BOOL | `true` | Event | — |
| `LEFT_JOY_X_THRESHOLD_RELEASED` | `0x1A` | BOOL | `false` | Event | — |
| `BUTTON_UP_DOWN_PRESSED` | `0x1B` | BOOL | `true` | Event | — |
| `BUTTON_UP_DOWN_RELEASED` | `0x1C` | BOOL | `false` | Event | — |

The names and meanings above MUST be preserved exactly. The physical joystick
and controller sensors belong to the controller; these event definitions do
not imply local robot GPIO inputs.

### 7.5 Reserved Response Events

| Event | ID | Type | Requirement |
|---|---:|---|---|
| `ACK` | `0xF0` | UINT8 | A valid, semantically accepted message was processed. Its value is the acknowledged request event ID. |
| `INFO` | `0xF1` | UINT16 | Reserved response event for requested information. Request semantics and value encoding are unresolved. |
| `ERR` | `0xF2` | UINT8 | A CRC-valid message was rejected as semantically invalid. Error value encoding is unresolved. |
| `REPT` | `0xF3` | UINT8 | The received frame had a CRC mismatch; requests retransmission. Value encoding is unresolved. |

Responses MUST use a valid protocol frame with a CRC calculated over the
response's `EVENT + TYPE + VALUE`. Responses MUST be returned on the ingress
channel and, for Wi-Fi, to the client/session that sent the request.

### 7.6 Event Transmission Rules

- Digital event signals MUST be emitted only on state changes. They MUST NOT be
  retransmitted continuously while the state remains unchanged.
- For any supported locally generated analog event, the first observed value
  MUST be transmitted. Subsequent values MUST be sent only when
  `abs(currentValue - lastSentValue) > eventThreshold`. Equality MUST NOT cause
  transmission. Update `lastSentValue` only after the retransmission is
  successfully handed to the transport.
- Analog thresholds MUST come from the event catalog.
- This robot firmware currently has no approved local source for the joystick
  or `SENSOR_X/Y/Z` events. It MUST NOT fabricate or transmit those values.

### 7.7 Forward Compatibility

The codec SHOULD use a type/field description table and clear separation between
frame parsing and dispatch so that byte stuffing and a sequence field can later
be added without replacing the entire codec. Byte stuffing and a sequence field
are explicitly out of scope for version 1.0 and MUST NOT be added to the current
wire format.

## 8. Application Event Handling

### 8.1 Validation and Dispatch

The event catalog MUST identify each supported event's expected type, value
validator, transmission metadata, and optional handler. The dispatcher MUST:

1. Receive only CRC-validated frames from `ProtocolCodec`.
2. Reject unknown event IDs with ERR.
3. Reject event/type mismatches with ERR.
4. Reject invalid values, including values outside the declared range, with
   ERR.
5. Invoke only the configured action for a valid event.
6. Send ACK after successful semantic handling, including valid events with no
   currently configured application action.
7. Never execute an action for CRC-invalid or semantically invalid input.

The `Events` module MUST allow event-to-action assignments to be changed in a
single application configuration/catalog location; event behavior MUST NOT be
spread across transport drivers.

### 8.2 Actions in Scope

The only currently approved command-to-output action is:

| Received event | Action |
|---|---|
| `BUTTON_UP_PRESSED` | Turn the information LED on. |
| `BUTTON_UP_RELEASED` | Turn the information LED off. |

The press/release pair controls a latched LED state. While this event-controlled
state is on, message activity MUST NOT turn the LED off or replace the steady
on state with a blink. After `BUTTON_UP_RELEASED`, the LED returns to its normal
message-activity behavior: blink on each received message and remain off between
activity indications.

Other valid catalog events MUST be validated and ACKed, but MUST NOT control
unspecified actuators. Adding robot movement or other actions requires an
explicit event-to-actuator mapping and hardware requirements.

## 9. Communication Transports

### 9.1 Wi-Fi AP and STA

AP and STA operation MUST be available concurrently, in addition to RF. Startup
MUST attempt to start the AP and connect to the configured STA network. The
service MUST continue to provide whichever Wi-Fi interface is operational if
the other fails. A Wi-Fi failure MUST NOT prevent RF initialization or firmware
loop operation.

The Wi-Fi transport MUST support protocol frames carried as a TCP byte stream.
TCP read boundaries MUST NOT be treated as frame boundaries. The codec MUST
accumulate bytes across reads and may emit multiple complete frames from one
read. Partial-frame state MUST be reset when its TCP peer disconnects.

For every accepted Wi-Fi request, the response MUST be sent to that originating
peer through its ingress interface (AP or STA). TCP writes MUST handle partial
write results and MUST NOT silently discard an unsent response. At minimum, the
service MUST avoid mixing data between distinct AP/STA clients. Multi-client
concurrency and queue limits are unresolved and are listed in Section 15.

Whenever a new AP station connects, log the client identity available from the
platform (at minimum MAC address when available). Log AP/STA connection and
disconnection events and STA IP assignment.

### 9.2 nRF24L01

The RF service MUST initialize the radio with the configured channel, address,
address width, power, and data rate, then enter receive mode. Each received RF
payload MUST be bounded by the radio's maximum payload size and validated before
accessing its contents.

An RF payload may contain one or more stream bytes; the implementation MUST
process each byte through the RF codec without exceeding the buffer or packet
length. RF responses MUST be transmitted to the requesting peer using the
configured return address and MUST return the radio to receive mode afterward.
RF transmit/receive operations MUST not block the main loop indefinitely.

### 9.3 Channel Independence

Wi-Fi AP, Wi-Fi STA, and RF MUST be serviced in the main loop without one
channel's partial frame state being reused by another. A request received on
one channel MUST be answered on that same channel; it MUST NOT be broadcast on
all channels.

## 10. Startup and Health State

Startup MUST initialize logging, GPIO outputs, Wi-Fi AP/STA, RF, and the
ultrasonic diagnostic sensor. While startup checks are running, the status LED
MUST blink.

The startup health checks MUST verify:

| Check | Passing condition |
|---|---|
| Wi-Fi AP | AP started and is available |
| Wi-Fi STA | Connected to the configured network and has an assigned IP |
| Ultrasonic sensor | Startup reading is non-zero |
| nRF24L01 | Radio responds and is configured in receive-ready state |

When all checks pass, the status LED MUST remain on. If any check fails, the
status LED MUST continue blinking after startup and each failure MUST be logged.
Failure of one component MUST NOT cause unrelated available channels to stop
serving requests. The system MUST retain and report an overall failed health
state while continuing to operate components that initialized successfully.

Because STA association may take longer than peripheral initialization, startup
MUST use the configured bounded connection timeout. A timeout is a failed STA
check; it MUST NOT hang boot indefinitely.

## 11. LED Behavior

### 11.1 Status LED

- Blink during startup health checks.
- Remain steadily on after successful startup.
- Continue blinking if one or more startup health checks fail.
- While running, a later loss of the connectivity condition used for health
  indication SHOULD cause blinking; recovery SHOULD restore the healthy steady
  indication. The exact health policy after boot MUST be consistent with the
  AP+STA simultaneous requirement and MUST not treat RF availability as a
  substitute for failed Wi-Fi AP/STA startup checks.

### 11.2 Information LED

- Stay steadily on from `BUTTON_UP_PRESSED` until `BUTTON_UP_RELEASED`.
- While not latched on by the button event, blink for each message received,
  whether that message is valid or invalid.
- A received message during the latched-on state MUST leave the LED on; message
  activity MUST NOT interfere with the latch.
- After the release event, resume the normal activity indication. An activity
  indication MUST be brief and non-blocking.
- The activity indication applies to inbound messages. The source notes do not
  require blinking for transmitted responses.

LED active polarity MUST be configurable or explicitly defined in `Config.hpp`.
LED changes MUST be non-blocking and MUST NOT delay protocol handling.

## 12. Logging

### 12.1 Logger API and Levels

`Logger` MUST expose an enable/disable setting and configurable `DEBUG`, `INFO`,
and `ERROR` levels. Compile-time defaults are required; a runtime setter is
recommended. It MUST provide `logDebug`, `logInfo`, and `logError` methods and
apply these filters:

| Configured level | Messages emitted |
|---|---|
| DEBUG | DEBUG, INFO, ERROR |
| INFO | INFO, ERROR |
| ERROR | ERROR only |

All methods MUST honor the global enable flag. The implementation MUST document
whether formatting is printf-style or `String`-based and MUST avoid unbounded
temporary allocations in frequent logging paths.

### 12.2 Required Operational Logs

The firmware MUST log:

- A startup banner and each startup-check result.
- AP SSID, password, and port; STA SSID, assigned IP, and port after connection.
- AP client connections and disconnections where observable.
- RF initialization and its effective configuration.
- Each received message on every channel, including malformed data that cannot
  be decoded as a frame. Raw bytes SHOULD be logged in bounded hexadecimal form.
- Each valid decoded event, including event name/ID and ingress channel.
- Semantic rejections, CRC mismatches, unsupported types, transmit failures,
  client lifecycle changes, and peripheral failures.
- The ultrasonic measurement once every second.

Logging malformed input MUST be bounded to prevent malformed or high-rate input
from exhausting memory or blocking the system. Credentials MUST NOT be included
in logs other than the specifically requested AP startup log.

## 13. Runtime and Reliability Requirements

- The main loop MUST continue servicing Wi-Fi, RF, timers, LEDs, and diagnostics
  without long blocking delays.
- Sensor measurement, LED activity, and network/radio waits MUST be bounded or
  asynchronous.
- Buffer sizes MUST be checked before every read, write, and frame encode.
- A malformed packet, invalid length, CRC mismatch, or client disconnect MUST
  not corrupt subsequent frame processing.
- A failed response transmission MUST be logged. The implementation MUST NOT
  report a successful send when the transport rejected or truncated the frame.
- State shared by Wi-Fi, RF, and event handling MUST have clear ownership; one
  transport's session reset MUST NOT reset another transport's parser.
- No protocol action is allowed before CRC and semantic validation both pass.

## 14. Acceptance Criteria and Test Requirements

Implementation is acceptable only when the following checks pass:

### 14.1 Build and Structure

- PlatformIO builds for `esp32-c3-devkitm-1`.
- All GPIO constants are centralized in `PinMap.hpp`.
- The specified modules and responsibilities are present without duplicated
  protocol, logging, or pin-map helpers.

### 14.2 Codec

- CRC16-MODBUS matches the standard known vector: ASCII `"123456789"` produces
  `0x4B37`.
- Encode/decode round trips succeed for every supported fixed-size type.
- Multi-byte numeric values use little-endian ordering.
- Frames fragmented at every possible byte boundary are decoded correctly.
- Multiple consecutive frames in one input stream are decoded separately.
- `0xAA` in value and CRC positions is treated as data after a frame begins.
- Bad CRC produces a CRC failure and no dispatch.
- Unknown types and unsupported variable-length payloads do not overrun buffers
  or dispatch partial data.

### 14.3 Semantic Dispatch

- Unknown event, wrong type, invalid boolean encoding, non-finite float, and
  out-of-range values are rejected without executing an action.
- Valid events without configured actions are ACKed without driving an output.
- `BUTTON_UP_PRESSED` turns the information LED on; subsequent received
  messages leave it on.
- `BUTTON_UP_RELEASED` turns the information LED off and returns it to normal
  activity indication.
- Each valid request receives an ACK, each CRC-valid semantic rejection
  receives an ERR, and each CRC mismatch requests retransmission with REPT,
  all on the same ingress channel.

### 14.4 Transports and Startup

- AP and STA can be operating concurrently with RF active.
- TCP partial reads, coalesced frames, disconnects, and fresh client sessions
  do not corrupt parser state.
- RF packets at minimum and maximum supported lengths are processed safely;
  malformed dynamic payload sizes are rejected.
- Startup passes each health check when the corresponding hardware/network is
  available and reports/blinks on each failed check.
- Failure of Wi-Fi does not disable RF reception; failure of RF does not
  disable Wi-Fi.
- The information LED activity indication is non-blocking, and the status LED
  reflects startup health as specified.
- Ultrasonic startup value is checked and measurements are logged once per
  second; a zero/timeout startup reading fails sensor initialization, and
  periodic readings use a bounded, non-blocking measurement state machine.
- Every active interface's received payload is logged in hexadecimal at DEBUG
  level and is decoded and dispatched through the common event path.

## 15. Unresolved Protocol and Operational Decisions

The implementation MUST not invent wire behavior for the items below. Before
claiming complete interoperability, these points require a protocol-owner
decision and an update to `protocol.yaml` and this document:

1. **Variable-length `BYTES`:** The frame has no length field, so a receiver
   cannot determine the end of a `BYTES` value before CRC. Define a length
   encoding, a fixed maximum/length rule, or formally declare `BYTES`
   unsupported in version 1.0. The current codec may reject it safely, but
   interoperability for it is not defined.
2. **`ERR` payload:** `ERR` is declared `UINT8` but no numeric error-code
   assignments or mapping from validation failures are defined.
3. **`REPT` payload:** `REPT` is declared `UINT8`, but its value is not defined
   (for example, whether it carries the original event ID or a reason).
4. **`INFO` semantics:** No request event, request value, or response-value
   encoding is defined.
5. **Wi-Fi concurrency:** The requirements mandate AP and STA at once, but do
   not define how many simultaneous TCP clients are required per interface,
   nor queue/backpressure policy. The implementation should document its
   supported client limit.
6. **RF response addressing:** The peer address/pipe configuration is named as
   configurable, but there is no over-the-air addressing contract between this
   firmware and the controller beyond use of the configured RF address.
7. **Health policy after boot:** AP and STA must both be attempted and checked,
   but behavior when STA is unavailable while AP is operational (and the exact
   running-state status LED policy) needs confirmation for a final health
   definition.

Until resolved, the implementation MUST use the explicitly defined fixed-size
types and event behaviors, reject unsupported/ambiguous inputs safely, and
document any chosen internal limitation. It MUST NOT silently assign new event
IDs or response payload meanings.

## 16. Source References

- `Notes.temp` provides the platform, pin map, target architecture, behavior,
  logging, startup, and initial protocol requirements.
- `protocol.yaml` defines EventProtocol v1.0 frame, CRC, type, and event IDs.
- The user clarified that this is robot-side receiver firmware, AP and STA must
  run simultaneously with RF, ultrasonic is a future integration beyond
  diagnostics, and the information LED is latched by the UP press/release pair
  while indicating received-message activity when not latched.
