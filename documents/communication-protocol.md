# Communication Protocol

## Purpose and scope

This document defines the application-level binary message format and the
responsibilities of a protocol parsing library. It describes messages
independently of the systems or devices that exchange them.

The message is a fixed-size 28-byte record. Event IDs and their meanings are
defined by the shared event catalog; the message itself does not include a
separate value-type field.

## Wire layout

All offsets are relative to the start of a message:

| Offset | Size | Field | Meaning |
| --- | ---: | --- | --- |
| 0 | 1 | `data[0]` | Event ID |
| 1 | 1 | `data[1]` | Value length in bytes |
| 2 | 18 | `data[2..19]` | Value bytes followed by unused bytes |
| 20 | 4 | `message_ID` | Signed 32-bit message ID, little-endian |
| 24 | 1 | `ID_sender` | One-byte sender identifier |
| 25 | 1 | `ID_receiver` | One-byte receiver identifier |
| 26 | 2 | Padding | Reserved bytes; encoded as zero |

The corresponding in-memory payload structure is:

```cpp
struct Payload {
    char data[20];
    int message_ID;
    char ID_sender;
    char ID_receiver;
};
```

The `data` array contains the event ID and value length in its first two bytes,
followed by up to 18 value bytes. Any unused value bytes are zero-filled by the
encoder. The two reserved bytes at the end of the wire record are also zero.
Implementations must serialize and deserialize the specified offsets
explicitly; the in-memory structure's compiler-specific padding is not itself
the wire format.

Integers use little-endian byte order. `FLOAT32` values, where applicable, use
little-endian IEEE-754 binary32 representation.

## Value encoding

The event definition associated with the event ID determines the value type
and expected length:

| Type | Wire representation | Validation |
| --- | --- | --- |
| `BOOL` | One byte: `00` for false, `01` for true | Exactly 1 byte; only 0 or 1 are valid |
| `FLOAT32` | Four bytes containing a binary32 float | Exactly 4 bytes; non-finite values are invalid |
| `BYTES` | Raw bytes | 0 through 18 bytes |

For example, `1.0` as `FLOAT32` is `00 00 80 3F`. Numeric event definitions
may additionally constrain the value with configured minimum and maximum
values. Such semantic checks belong to event validation, not binary
deserialization.

## Message IDs

New messages use a signed 32-bit message ID beginning at 1 and incrementing
for each message. After `INT32_MAX`, the next ID is 1. A response echoes the
message ID of the message it answers. The protocol parser handles the ID as a
field; request tracking and response matching belong to the event or
application layer.

## Protocol library functions

The protocol library provides two core operations:

### Decode a received message

The decoder receives the complete message as bytes and returns a `Payload`
structure on success:

```cpp
Result<Payload, DecodeError> decodePayload(ByteSpan message);
```

It requires exactly 28 bytes, reads each field from its defined offset,
decodes the 32-bit message ID as little-endian, and checks that the declared
value length is no greater than 18 before returning the structure. A
wrong-sized message or invalid length is a decoding error; it must not be
returned as a successful payload. The decoder does not look up event
definitions or validate event-specific value types and constraints.

### Encode a payload

The encoder receives the event ID, value bytes, message ID, sender ID, and
receiver ID, and returns the serialized 28-byte message:

```cpp
Result<ByteArray<28>, EncodeError> encodePayload(
    EventId eventId,
    ByteSpan value,
    int32_t messageId,
    uint8_t senderId,
    uint8_t receiverId);
```

It rejects values longer than 18 bytes, writes the event ID and value length
into `data[0]` and `data[1]`, places the value starting at `data[2]`, writes
the remaining fields at the offsets in the wire-layout table, and zero-fills
all unused value and reserved bytes. Event-specific value validation is
performed by the event library before encoding.

The signatures above describe the required behavior; concrete byte-span,
result, and error type names may follow the implementation language and
project conventions.

## Record boundaries and limitations

Each message is one 28-byte record. A byte stream that supplies partial or
combined records must be buffered by the caller until complete records are
available, then passed to the decoder one record at a time. Incomplete bytes
are not a message and must not be decoded.

The message has no application checksum, authentication, or encryption field.
It must not be treated as a secure or corruption-proof format.

## Examples

### Boolean value

Event ID `0x01`, boolean true, message ID 1, sender ID `0x31`, and receiver ID
`0x00` serialize as:

```text
Offset 00-19 (data): 01 01 01 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
Offset 20-23 (ID):   01 00 00 00
Offset 24 (sender):  31
Offset 25 (receiver):00
Offset 26-27 (pad):  00 00
```

The first byte is the event ID, the second byte is the value length, and the
third byte is the boolean value.

### Floating-point value

Event ID `0x0F`, `FLOAT32` value `2048.0`, message ID 2, sender ID `0x31`, and
receiver ID `0x00` serialize as:

```text
Offset 00-19 (data): 0F 04 00 00 00 45 00 00 00 00 00 00 00 00 00 00 00 00 00 00
Offset 20-23 (ID):   02 00 00 00
Offset 24 (sender):  31
Offset 25 (receiver):00
Offset 26-27 (pad):  00 00
```

The four value bytes `00 00 00 45` are the little-endian binary32 encoding of
`2048.0`.

## Source of truth

The current payload structure and protocol implementation are defined in
`src/includes/ProtocolTypes.hpp` and `src/lib/protocol/ProtocolHandler.cpp`.
The event catalog, which assigns meanings and types to event IDs, is in
`SDFiles/events.yaml`.
