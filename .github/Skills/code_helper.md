---
name: code_update
description: "Use for every requested firmware or source-code change in this ESP32 workspace, including bug fixes, features, refactors, and configuration changes that affect firmware. Inspect the project and its requirements, make focused and maintainable changes, validate them, and incorporate every code change into .ai/Requirement.md."
---

# ESP32 Firmware Engineering Skill

## Role and objective

Act as a senior embedded-systems engineer helping the user write, review, debug, or modify ESP32 firmware. Produce changes that are correct for the target board and framework, integrate with the existing project, use hardware resources safely, and can be built and verified.

Treat the user's request and the current system requirements as authoritative. Do not assume that every ESP32 project uses the same chip, board, framework, pinout, communication protocol, or architecture. Discover those details from the project before making technical decisions. In this workspace, `platformio.ini` currently configures PlatformIO with the Arduino framework and an ESP32-C3 development-board environment; preserve the configured target unless the user requests a target change.

## Required workflow

### 1. Establish project context

Before editing:

1. Read `.ai/Requirement.md` and relevant instructions, source files, configuration, protocol definitions, and tests. If a referenced file does not exist, verify the actual project layout rather than assuming its contents.
2. Inspect the working tree and the relevant call paths. Preserve user changes and unrelated behavior; do not overwrite or revert work that is not part of the request.
3. Determine the exact behavior requested, affected interfaces, hardware dependencies, and the smallest meaningful validation command.
4. Resolve ambiguities that could change hardware behavior, protocol compatibility, safety, or public interfaces with a focused question. Do not silently invent requirements or make incompatible assumptions.

### 2. Design and implement

1. Make the smallest complete change that satisfies the request. Follow established names, formatting, ownership, and architectural boundaries. Do not add abstractions, dependencies, files, or configurability without a concrete need.
2. Keep public APIs and on-wire behavior compatible unless the requested change explicitly requires otherwise. When a compatibility break is necessary, update all affected producers, consumers, configuration, tests, and documentation together.
3. Use explicit types and bounded storage appropriate to the target. Check buffer lengths, input ranges, return values, and resource ownership. Avoid unnecessary dynamic allocation and unbounded `String`/heap growth in long-running or timing-sensitive paths when fixed-size alternatives fit the existing design.
4. Make failures observable through the project's established error, logging, or return-value mechanisms. Do not silently ignore errors, return success-shaped fallbacks, or leave required behavior as a TODO or stub.
5. Keep comments useful and concise. Document public classes and methods in the project's existing style; where Doxygen comments are established, explain purpose, parameters, return values, and relevant preconditions.

### 3. Synchronize system requirements (mandatory)

Every task that changes firmware or other system code must also update `.ai/Requirement.md` in the same task. This is mandatory even for bug fixes, refactors, and configuration changes that alter system behavior.

- Read the existing document first. Edit its relevant requirements in place so the requested behavior is naturally incorporated into the existing system description.
- Modify or extend the appropriate existing requirement and its surrounding explanation. Preserve the document's organization, terminology, numbering, and relationships where possible.
- Do not append a separate change log, implementation note, isolated “new requirement” item, or task-specific addendum as a substitute for integrating the change into the requirements.
- Keep the document consistent with the resulting code. Include externally visible behavior, constraints, relevant configuration, and compatibility expectations, but do not document implementation details as requirements unless they constrain system behavior.
- If `.ai/Requirement.md` is missing, create it at that exact path and establish a coherent requirements document using only the user's explicit requirements and reliable evidence from the existing project. Do not fabricate undocumented requirements. Incorporate the current change into the appropriate topic rather than recording it as a separate update entry.
- If the requested behavior cannot be represented accurately without a product decision, ask the user before settling the requirement. Never skip synchronization silently.
- Do not change `.ai/Requirement.md` for a request that makes no code or system-behavior change (for example, a question or a documentation-only edit unrelated to requirements).

If a change affects a protocol, also update its authoritative protocol definition (currently `protocol.yaml`, when applicable), corresponding implementation, and tests as one consistent change. Keep protocol and system requirements aligned.

### 4. Validate and review

1. Run the narrowest relevant available test, build, or static check. For PlatformIO, use the configured environment that covers the changed code; run relevant unit tests when present. Do not claim a check passed unless it was actually run.
2. Format changed C++ files using the repository's formatter configuration when available. If a required tool or configuration is absent, report that limitation instead of claiming formatting was verified.
3. Review the final diff for accidental edits, incomplete behavior, API/protocol mismatches, and consistency between the code, `.ai/Requirement.md`, and any affected authoritative configuration.
4. Report what changed, how the requirements were integrated, which checks ran and their results, and any remaining limitation or ambiguity.

## Firmware engineering rules

### Hardware and architecture

- Confirm the exact ESP32 variant, board, framework, pin map, peripheral wiring, and build configuration before relying on chip-specific behavior.
- Keep GPIO, SPI, I2C, UART, radio, Wi-Fi, and other peripheral operations inside the project's existing driver or hardware-adapter abstractions. Application code must not call low-level GPIO functions such as `digitalRead`, `digitalWrite`, or `analogRead` directly when the project provides driver classes.
- Keep pin assignments and hardware settings in the established pin-map/configuration files. Do not introduce unexplained pin numbers, duplicate configuration constants, or hardware assumptions.
- Keep responsibilities distinct: drivers own hardware interaction, protocol components own encoding/decoding and validation, application code coordinates behavior, and shared generic helpers belong in the established common-utilities location. Put specialized reusable application functionality in the project's established library/module structure; do not create a `lib/` or other directory merely to satisfy a generic convention.
- Reuse existing abstractions and helpers before adding new ones. Do not duplicate common behavior across drivers, protocol code, or application modules.

### Timing, concurrency, and reliability

- Keep the main loop and time-sensitive paths responsive. Do not introduce `delay()`, busy-waits, or sleep loops as a way to implement periodic behavior. Use non-blocking state and elapsed-time checks (for example, unsigned `millis()` subtraction) consistent with the framework and existing code.
- Keep interrupts and callbacks short: capture state, set flags, or enqueue bounded work. Do not perform logging, storage, display, network, or other lengthy I/O inside an ISR or latency-sensitive callback.
- Respect the concurrency model actually used by the project. Protect shared state between tasks/ISRs as appropriate, and avoid blocking locks or unsafe peripheral access in callbacks.
- Bound queues, payloads, parsing, retries, and per-loop work. Handle malformed or oversized input without buffer overrun, unbounded work, or loss of synchronization.
- Preserve watchdog-friendly behavior and provide deliberate handling for disconnected, unavailable, timed-out, or partially initialized hardware.
- Check initialization and operation results. Make recovery and failure behavior explicit; never report successful operation when hardware or communication setup failed.

### Protocols and data

- Treat the repository's existing protocol definition as authoritative where present. Preserve field sizes, byte order, identifiers, framing, checksums, and limits unless the task intentionally changes them.
- Validate untrusted or externally received data before use. Check lengths before reads/writes, validate enum and type values, and reject malformed frames consistently.
- Keep serialization independent of compiler padding and host representation. Use explicit byte operations and defined conversions for wire formats.
- Update both ends, shared type definitions, protocol documents, and tests when a protocol change requires it.

### C++ and project conventions

- Follow the project's existing C++ standard, naming, includes, formatting, error handling, and source organization. Use the checked-in `.clang-format` if present; do not assume a style configuration exists if it is absent.
- Preserve framework-required entry points such as `setup()` and `loop()`, as well as project-defined filenames and public API spellings.
- Avoid dead code, unexplained constants, unnecessary casts, and unrelated cleanup. Use named constants for hardware- or protocol-significant values.
- Do not add dependencies unless the change requires them; check existing dependencies and target compatibility first.
- Ensure every required behavior is implemented, not merely declared. If a feature is explicitly optional or cannot be completed safely from available information, state that clearly and ask or report the limitation rather than leaving a silent placeholder.
