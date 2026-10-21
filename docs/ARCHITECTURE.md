# Architecture

## System context

RadixLab has four concrete layers and one deliberate protocol boundary.

```text
┌──────────────────────────────────────────────────────────────┐
│ Browser: exact converter + interactive Encoding Lab          │
└──────────────────────────────┬───────────────────────────────┘
                               │ JSON over HTTP
┌──────────────────────────────▼───────────────────────────────┐
│ Spring Boot                                                  │
│                                                              │
│ ConversionController          EncodingController             │
│   └─ ConversionService          └─ EncodingService           │
│        └─ RadixEngine                 └─ EncodingEngine      │
│             └─ CliRadixEngine              └─ CliEncodingEngine│
│                        └──────────┬──────────────┘           │
│                            CliEngineClient                   │
│                              ├─ shared Semaphore              │
│                              ├─ shared timeout policy         │
│                              └─ ProcessExecutor ◄── fake tests│
│                                   └─ DefaultProcessExecutor   │
└──────────────────────────────┬───────────────────────────────┘
                               │ argv, stdin unused; JSON stdout/stderr
┌──────────────────────────────▼───────────────────────────────┐
│ radixlab-cli                                                 │
│   ├─ conversion: RationalParser → RadixConverter             │
│   └─ encoding: BitVector, signed, BCD, Gray, parity          │
└──────────────────────────────────────────────────────────────┘
```

There is no shell in the Java-to-C++ path. The CLI is a stable, testable process
interface rather than an RPC framework or JNI bridge. Conversion and encoding
share one process client, so they also share timeouts and process-slot limits.

## C++ core

### `RadixParser`

Parses integers in bases 2–36 into `std::int64_t`. It performs Horner
accumulation in `std::uint64_t` and checks overflow before multiplication. The
negative limit is one larger than the positive limit so `INT64_MIN` is handled
without undefined behavior. This parser demonstrates checked fixed-width parsing;
the converter intentionally uses arbitrary precision instead.

### `Rational`

Stores a numerator and denominator as `boost::multiprecision::cpp_int`.

Invariants enforced by the constructor:

1. The denominator is non-zero.
2. The denominator is positive.
3. Numerator and denominator are reduced by their greatest common divisor.
4. Zero is always `0/1`.

No floating-point type appears in the parsing or conversion path.

### `RationalParser`

Parses `[+-]? digits [ "." digits ]` in bases 2–36. A numeral must contain at
least one digit; a leading or trailing radix point is allowed when digits exist
on the other side. Each digit after the point multiplies the denominator by the
source base. Digit validation is shared with `RadixParser` and `RadixConverter`
through `radixlab/digits.hpp`.

Examples:

```text
101101.011₂ = 363/8
2AF.8₁₆     = 1375/2
0.1₁₀      = 1/10
```

### `RadixConverter`

Conversion has two stages.

**Integer stage.** Divide the integer magnitude repeatedly by the target base,
collect the remainders as digits, then reverse them. Zero is emitted as `0`.

**Fraction stage.** Let `r` be the remainder after the integer division. Until
`r` is zero or a limit is reached:

1. If `r` has been seen before, a cycle starts at its first recorded index.
2. If the output has reached `maxFractionDigits`, mark the result truncated.
3. Record `r` and its digit index.
4. Compute `scaled = r * targetBase`.
5. Emit `scaled / denominator` as the next digit.
6. Set `r = scaled % denominator`.

The denominator is constant during conversion, so a repeated remainder implies
the subsequent digit sequence repeats exactly. The cycle is formatted as
`prefix(cycle)`.

Result semantics:

| Condition | `exact` | `repeating` | `truncated` |
| --- | --- | --- | --- |
| Remainder reaches zero | `true` | `false` | `false` |
| Remainder repeats | `true` | `true` | `false` |
| Limit reached first | `false` | `false` | `true` |

A repeat is checked before the output limit. This allows a complete one-digit
cycle such as `0.(3)` to be reported at a fractional limit of one.

### Resource bounds

The library itself accepts a caller-provided fractional limit. The CLI and API
enforce a maximum conversion input length of 4096 and a maximum fractional limit
of 4096. Encoding width is capped at 64 bits and BCD input at 1024 digits.
These are application policy limits, not mathematical limits.

## Encoding core

The v0.2.0 module separates a bit pattern from its interpretation.

### `BitVector`

`BitVector` stores an unsigned `cpp_int` pattern and a width from 1 to 64. It is
not a signed number. It validates that the pattern fits the width, renders binary
and hexadecimal forms, reads indexed bits (index 0 is LSB), performs logical
shifts, masks bitwise NOT to the declared width, and supports bit set/flip. All
masks use `cpp_int`, avoiding 64-bit signed overflow and shifting past a native
integer.

Logical shifts define a complete result: vacant bits are zero, discarded high
bits are truncated, and a shift by at least the width is zero. Signed behavior is
never implicit; it belongs to a signed encoding.

### Signed encodings

`SignedEncoding` selects sign-magnitude, ones' complement, or two's complement.
Ranges are calculated from the width and checked before encoding. Overflow raises
`RangeError`; it is never handled by masking or truncating the input.

For width `n`, the minimum and maximum are:

- Sign-magnitude / ones' complement: `-(2^(n-1)-1)` through `2^(n-1)-1`.
- Two's complement: `-2^(n-1)` through `2^(n-1)-1`.

Sign-magnitude and ones' complement have two bit patterns for zero. The API
exposes the distinction with `negativeZero` for both encoding and decoding, while
two's complement rejects a negative-zero request. Decoding a sign-magnitude
`10000000` or ones' complement all-ones word therefore returns numeric value zero
with `negativeZero=true` rather than losing that information.

### BCD

`encodeBcd` accepts a non-empty string of decimal digits and preserves leading
zeros. Each digit is emitted as a 4-bit group; `value` is accumulated separately
for numerical comparison and for an ordinary binary rendering. `decodeBcd` keeps
group boundaries, accepts optional spaces for readability, requires a non-empty
multiple of four bits, and rejects any nibble outside `0000-1001`. This version
defines no negative BCD protocol.

### Gray code

`binaryToGray` computes `G = B xor (B >> 1)`. `grayToBinary` reconstructs the
binary word from the most-significant bit downward. Width is explicit, values are
unsigned, and conversion is integer-only. Tests verify round trips and that every
adjacent pair in the 4-bit and 8-bit sequences differs in exactly one bit.

### Parity

`generateParity` counts the ones and emits the bit that makes the data-plus-parity
word even or odd. The parity bit is appended at the right in the string form. The
CLI/API also expose a single-bit flip experiment: the original and received data
are both reported, and the receiver recomputes parity independently of the
transmitted bit. The model intentionally documents that single-bit parity cannot
detect all even numbers of errors and cannot locate or correct an error.

## CLI contract

```bash
radixlab-cli \
  --input 101101.011 \
  --from 2 \
  --to 16 \
  --max-fraction-digits 256 \
  --json

radixlab-cli encoding \
  --operation signed-encode \
  --value -13 --width 8 \
  --encoding TWOS_COMPLEMENT \
  --json
```

The `encoding` namespace currently supports `signed-encode`, `signed-decode`,
`bcd-encode`, `bcd-decode`, `gray-encode`, `gray-decode`, and `parity`. See
`radixlab-cli encoding --help`.

Success writes exactly one JSON object to stdout:

```json
{
  "input": "101101.011",
  "sourceBase": 2,
  "targetBase": 16,
  "result": "2D.6",
  "exact": true,
  "truncated": false,
  "repeating": false,
  "maxFractionDigits": 256
}
```

Without `--json`, stdout contains a concise result. Errors go to stderr. When
`--json` is present, errors are JSON even if argument parsing failed. Exit codes
are `0` success, `1` internal failure, `2` invalid input/arguments, `3` resource
limit, and `4` value outside the representable encoding range.

## Java backend

### Request pipeline

1. Jackson binds the JSON body.
2. Jakarta Bean Validation enforces shape, length, base, and bit constraints.
3. `ConversionService` applies the default fraction limit (256).
4. `CliRadixEngine` or `CliEncodingEngine` builds an argument vector.
5. `CliEngineClient` acquires a process slot and invokes `ProcessExecutor`.
6. stdout and stderr are drained concurrently on virtual threads.
7. A timeout terminates the child, first gracefully and then forcibly.
8. Exit code and JSON determine the result or a typed exception.
9. `ApiExceptionHandler` maps failures to client-safe HTTP errors.

### Replaceable boundaries

`RadixEngine` is the service boundary. `ConversionServiceTest` and `EncodingServiceTest` substitute them without
touching processes. `ProcessExecutor` is the OS boundary. `CliRadixEngineTest`
and `CliEncodingEngineTest` substitute it to verify command construction, output
parsing, timeout mapping, and range mapping. `ConversionIntegrationTest` and
`EncodingIntegrationTest` use the real C++ binary through HTTP.

### Failure mapping

| Cause | Exception | HTTP |
| --- | --- | --- |
| Bean validation failure | `MethodArgumentNotValidException` | 400 |
| Malformed JSON | `HttpMessageNotReadableException` | 400 |
| CLI input rejection | `InvalidNumberException` / `InvalidEncodingException` | 400 |
| Representable-range rejection | `UnrepresentableValueException` | 422 |
| Process timeout | `EngineTimeoutException` | 504 |
| No process slot | `EngineBusyException` | 503 |
| Start/read/abnormal exit/JSON failure | `EngineFailureException` | 502 |

Client responses contain stable messages. Engine stderr and stack traces are
logged server-side only. The whole path uses argument vectors, so characters
such as `;`, `$()`, quotes, or whitespace in a numeral cannot become shell
syntax.

## Front end

Spring Boot serves `src/main/resources/static/index.html`, the v0.2.0
`encoding.html` Encoding Lab, and their assets. Neither page uses a framework or
external network assets. The converter:

- populates bases 2–36 locally;
- validates the numeral shape and source-base digits before sending;
- disables the submit button while an HTTP request is in flight;
- aborts the client request after 12 seconds;
- renders the result with `textContent`, never by injecting HTML;
- distinguishes repeating and truncated output;
- exposes errors in a live region and uses visible focus styles.

The Encoding Lab adds a keyboard-operable tab list and four live explorers:
signed encode/decode, BCD groups, Gray transitions with changed-bit highlighting,
and parity generation with an optional single-bit flip. Results are rendered with
`textContent`, not injected HTML.

## Testing strategy

| Layer | Tests |
| --- | --- |
| C++ fixed-width parser | 25 assertions: signs, boundaries, invalid input, overflow |
| Rational | 9 assertions: normalization, signs, zero, arbitrary precision |
| Rational parser | 18 assertions: bases, fractions, malformed input |
| Converter | 32 assertions: examples, signs, cycles, truncation, large values |
| BitVector | 56 assertions: widths, reads, strings, shifts, masks, validation |
| Signed encodings | 52 assertions: three forms, ranges, negative zero, 4–64 bits |
| BCD | 26 assertions: groups, leading zeros, illegal nibbles, comparison |
| Gray code | 17 assertions: known sequence, adjacency, round trips, 64-bit boundary |
| Parity | 23 assertions: even/odd, flips, undetected double flips, 64-bit |
| CLI process | 25 checks: conversion/encoding output, JSON, exit codes, limits |
| Java unit | Service mapping, CLI adapters, process adapter, concurrency |
| Java web | `@WebMvcTest` validation and error statuses for both API families |
| Integration | HTTP → Spring → real CLI → C++ conversion and encoding, plus both UIs |

Total: 258 C++ assertions, 25 CLI checks, and 55 JUnit tests.

Tests do not rely on `assert()` being enabled, so Debug and Release C++ builds
run the same checks. CI rejects failures rather than ignoring them.

## Deliberate trade-offs

### Process per request

This MVP starts one CLI process per conversion or encoding request. It provides a small, strongly
isolated interface and a language boundary that is trivial to inspect and test.
The costs are process-start overhead, a fixed concurrency ceiling, and the need
to supervise every child process. `Semaphore`, timeouts, forced termination, and
drained output streams make those costs explicit.

An in-process FFI/JNI implementation would remove process overhead but introduce
native memory management, shared-library packaging, and crash propagation across
the JVM boundary. That is intentionally deferred.

### Exact rationals, not decimals

`cpp_int` rationals allow any finite positional input to be represented exactly.
The converter performs only integer arithmetic. This avoids binary floating-point
rounding but can make numerators large for long inputs, which is why external
input limits exist.

### JSON through stdout

The process protocol is inspectable and works in every shell. It avoids port
allocation and RPC lifecycle management. The trade-off is parsing overhead and
the need to keep stdout reserved strictly for one result object.

Both API families share the same `CliEngineClient`, preserving one timeout
policy, one process-slot semaphore, and one test seam.
