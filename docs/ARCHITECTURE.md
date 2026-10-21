# Architecture

## System context

RadixLab has four concrete layers and one deliberate protocol boundary.

```text
┌──────────────────────────────────────────────────────────────┐
│ Browser: semantic form, validation, result states            │
└──────────────────────────────┬───────────────────────────────┘
                               │ JSON over HTTP
┌──────────────────────────────▼───────────────────────────────┐
│ Spring Boot                                                  │
│                                                              │
│ ConversionController                                         │
│   └─ ConvertRequest / ConvertResponse                        │
│ ConversionService                                            │
│   └─ RadixEngine  ◄──────────── Fake engine in unit tests    │
│        └─ CliRadixEngine                                     │
│             ├─ Semaphore (max concurrent processes)          │
│             └─ ProcessExecutor ◄── fake in adapter tests     │
│                  └─ DefaultProcessExecutor                   │
└──────────────────────────────┬───────────────────────────────┘
                               │ argv, stdin unused; JSON stdout/stderr
┌──────────────────────────────▼───────────────────────────────┐
│ radixlab-cli                                                 │
│   └─ RadixConverter                                          │
│        └─ RationalParser ─► Rational ─► cpp_int arithmetic   │
└──────────────────────────────────────────────────────────────┘
```

There is no shell in the Java-to-C++ path. The CLI is a stable, testable process
interface rather than an RPC framework or JNI bridge.

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
enforce a maximum input length of 4096 and a maximum fractional limit of 4096.
These are application policy limits, not mathematical limits.

## CLI contract

```bash
radixlab-cli \
  --input 101101.011 \
  --from 2 \
  --to 16 \
  --max-fraction-digits 256 \
  --json
```

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

Without `--json`, stdout contains the result and a newline. Errors go to stderr.
When `--json` is present, errors are JSON even if argument parsing failed. Exit
codes are `0` success, `1` internal failure, `2` invalid input/arguments, and
`3` resource limit.

## Java backend

### Request pipeline

1. Jackson binds the JSON body.
2. Jakarta Bean Validation enforces shape, length, and base ranges.
3. `ConversionService` applies the default fraction limit (256).
4. `CliRadixEngine` acquires a process slot.
5. `ProcessExecutor` starts the CLI with an argument vector.
6. stdout and stderr are drained concurrently on virtual threads.
7. A timeout terminates the child, first gracefully and then forcibly.
8. Exit code and JSON determine the result or a typed exception.
9. `ApiExceptionHandler` maps failures to client-safe HTTP errors.

### Replaceable boundaries

`RadixEngine` is the service boundary. `ConversionServiceTest` substitutes it
without touching processes. `ProcessExecutor` is the OS boundary.
`CliRadixEngineTest` substitutes it to verify command construction, output
parsing, timeout mapping, and capacity handling deterministically.
`ConversionIntegrationTest` uses the real C++ binary through HTTP.

### Failure mapping

| Cause | Exception | HTTP |
| --- | --- | --- |
| Bean validation failure | `MethodArgumentNotValidException` | 400 |
| Malformed JSON | `HttpMessageNotReadableException` | 400 |
| CLI input rejection | `InvalidNumberException` | 400 |
| Process timeout | `EngineTimeoutException` | 504 |
| No process slot | `EngineBusyException` | 503 |
| Start/read/abnormal exit/JSON failure | `EngineFailureException` | 502 |

Client responses contain stable messages. Engine stderr and stack traces are
logged server-side only. The whole path uses argument vectors, so characters
such as `;`, `$()`, quotes, or whitespace in a numeral cannot become shell
syntax.

## Front end

Spring Boot serves `src/main/resources/static/index.html` and its assets. The
page uses no framework and no external network assets. It:

- populates bases 2–36 locally;
- validates the numeral shape and source-base digits before sending;
- disables the submit button while an HTTP request is in flight;
- aborts the client request after 12 seconds;
- renders the result with `textContent`, never by injecting HTML;
- distinguishes repeating and truncated output;
- exposes errors in a live region and uses visible focus styles.

## Testing strategy

| Layer | Tests |
| --- | --- |
| C++ fixed-width parser | 25 assertions: signs, boundaries, invalid input, overflow |
| Rational | 9 assertions: normalization, signs, zero, arbitrary precision |
| Rational parser | 18 assertions: bases, fractions, malformed input |
| Converter | 32 assertions: examples, signs, cycles, truncation, large values |
| CLI process | 14 checks: plain/JSON output, exit codes, limits, stderr |
| Java unit | Service mapping, CLI adapter, process adapter, concurrency |
| Java web | `@WebMvcTest` validation and error statuses |
| Integration | HTTP → Spring → real CLI → C++ engine, plus static UI |

Tests do not rely on `assert()` being enabled, so Debug and Release C++ builds
run the same checks. CI rejects failures rather than ignoring them.

## Deliberate trade-offs

### Process per request

This MVP starts one CLI process per conversion. It provides a small, strongly
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
