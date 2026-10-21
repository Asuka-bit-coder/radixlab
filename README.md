# RadixLab

**Exact numeral conversion between bases 2 and 36, from the command line to the browser.**

RadixLab is an educational systems-software project built around one strict
idea: converting a number should never silently turn it into a floating-point
approximation. A C++20 engine parses positional numerals into arbitrary-precision
rational numbers, converts them exactly, detects repeating cycles, and reports
when an output limit forced truncation. A Java 21 / Spring Boot service exposes
that engine over HTTP and serves a lightweight web interface.

```
101101.011₂  →  2D.6₁₆        exact, terminating
0.1₁₀        →  0.0(0011)₂    exact, repeating cycle
```

## Features

- Bases 2 through 36, with uppercase output and case-insensitive input.
- Signed integers and fractions, including negative zero normalization.
- Arbitrary-precision C++ integers via Boost.Multiprecision; no `double`
  intermediate representation.
- Exact rational normalization: positive denominator, reduced terms, `0/1` for
  zero, and rejection of a zero denominator.
- Repeating-cycle detection by tracking previously seen remainders.
- Three explicit result states: terminating, repeating, and truncated.
- Standalone `radixlab-cli` with plain-text or structured JSON output.
- Spring Boot REST API using a replaceable process-execution boundary.
- Process timeouts, bounded concurrent process slots, input limits, and safe
  error responses.
- Responsive, keyboard-accessible web UI with no front-end framework.
- CTest, JUnit, process-level CLI tests, and a real Java-to-C++ integration suite.
- GitHub Actions jobs for C++, Java, and end-to-end verification.

## Exactness contract

RadixLab distinguishes three outcomes and never labels a truncated value exact:

| Outcome | `exact` | `repeating` | `truncated` | Example |
| --- | --- | --- | --- | --- |
| Terminating | `true` | `false` | `false` | `0.5₁₀ = 0.1₂` |
| Complete cycle | `true` | `true` | `false` | `0.1₁₀ = 0.0(0011)₂` |
| Limit reached | `false` | `false` | `true` | `0.1₁₀ → 0.00₂` at limit 2 |

Cycle digits are enclosed in parentheses. A truncated result contains only the
digits emitted within the limit; callers must inspect the `truncated` flag.

## Architecture

```text
Browser
  │  POST /api/v1/convert
  ▼
Spring Boot (Java 21)
  ├─ ConvertRequest validation
  ├─ ConversionService
  ├─ RadixEngine interface
  │    └─ CliRadixEngine
  │         ├─ Semaphore (bounded concurrency)
  │         └─ ProcessExecutor
  │              └─ ProcessBuilder, no shell
  ▼
radixlab-cli (C++20)
  ├─ RationalParser ──► Rational (cpp_int)
  └─ RadixConverter ──► terminating / repeating / truncated
```

The CLI is the process protocol between Java and C++. The request is passed as
an argument vector, never interpolated into a shell command. The CLI returns one
JSON object on stdout and uses stderr for errors, with stable exit codes:

| Exit code | Meaning |
| --- | --- |
| `0` | Success |
| `1` | Internal engine failure |
| `2` | Invalid arguments or numeral |
| `3` | Input resource limit exceeded |

See [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) for algorithms, boundaries, and
design trade-offs.

## Technology stack

| Layer | Technology |
| --- | --- |
| Exact arithmetic | C++20, Boost.Multiprecision `cpp_int` |
| C++ build/test | CMake, CTest |
| CLI | C++20, CMake |
| Backend | Java 21, Spring Boot 3.5 |
| Build/test | Maven, JUnit 5, Mockito, AssertJ |
| Web UI | Semantic HTML, CSS, vanilla JavaScript |
| CI | GitHub Actions |

## Prerequisites

- CMake 3.20 or newer.
- A C++20 compiler (Apple Clang, Clang, or GCC).
- Boost 1.70 or newer, including its CMake package configuration.
- Java 21 and Maven 3.9 or newer for the backend.
- macOS or Linux for the tested process-control paths.

Install Boost on macOS with `brew install boost`. On Debian/Ubuntu, install
`libboost-dev`.

## Build and test the C++ engine

```bash
cmake -S cpp-core -B cpp-core/build -DCMAKE_BUILD_TYPE=Release
cmake --build cpp-core/build --parallel
ctest --test-dir cpp-core/build --output-on-failure
```

For a Debug build, use `-DCMAKE_BUILD_TYPE=Debug`. The suites use executable
tests and return non-zero on failure; they do not depend on `assert()` being
enabled.

Run the CLI directly:

```bash
./cpp-core/build/radixlab-cli \
  --input 101101.011 --from 2 --to 16
# 2D.6

./cpp-core/build/radixlab-cli \
  --input 0.1 --from 10 --to 2
# 0.0(0011)

./cpp-core/build/radixlab-cli \
  --input 101101.011 --from 2 --to 16 --json
{"input":"101101.011","sourceBase":2,"targetBase":16,"result":"2D.6","exact":true,"truncated":false,"repeating":false,"maxFractionDigits":256}
```

Limit the fractional output:

```bash
./cpp-core/build/radixlab-cli \
  --input 0.1 --from 10 --to 2 --max-fraction-digits 2
# 0.00
```

## Run the Java backend and web UI

Build the CLI first, then start Spring Boot:

```bash
cd java-backend
RADIXLAB_ENGINE_CLI_PATH=../cpp-core/build/radixlab-cli \
  mvn spring-boot:run
```

Open <http://localhost:8080>. To build and run the executable jar instead:

```bash
cd java-backend
mvn package
RADIXLAB_ENGINE_CLI_PATH=../cpp-core/build/radixlab-cli \
  java -jar target/radixlab-backend-0.1.0.jar
```

Configuration:

| Property | Environment variable | Default |
| --- | --- | --- |
| `radixlab.engine.cli-path` | `RADIXLAB_ENGINE_CLI_PATH` | `radixlab-cli` on `PATH` |
| `radixlab.engine.timeout` | `RADIXLAB_ENGINE_TIMEOUT` | `5s` |
| `radixlab.engine.max-concurrent` | `RADIXLAB_ENGINE_MAX_CONCURRENT` | `4` |

## API

`POST /api/v1/convert`

```bash
curl -sS http://localhost:8080/api/v1/convert \
  -H 'Content-Type: application/json' \
  -d '{
    "number": "101101.011",
    "fromBase": 2,
    "toBase": 16,
    "maxFractionDigits": 256
  }'
```

```json
{
  "result": "2D.6",
  "exact": true,
  "truncated": false,
  "repeating": false
}
```

Request validation:

- `number`: required, 1–4096 characters, optional sign, at most one radix point.
- `fromBase` and `toBase`: required integers from 2 through 36.
- `maxFractionDigits`: optional integer from 0 through 4096; defaults to 256.

Errors use a stable shape:

```json
{
  "timestamp": "2026-10-21T10:00:00Z",
  "status": 400,
  "error": "Bad Request",
  "message": "fromBase must be between 2 and 36",
  "path": "/api/v1/convert"
}
```

HTTP statuses include `400` for invalid requests, `502` for an engine failure,
`503` when all engine slots are busy, and `504` for an engine timeout. Internal
paths and stack traces are not returned to clients. Full request and response
semantics are in [docs/API.md](docs/API.md).

## Test everything

```bash
# C++ unit, converter, and CLI process tests
ctest --test-dir cpp-core/build --output-on-failure

# Java unit and controller tests
cd java-backend
mvn test

# Java unit tests plus real Java -> C++ HTTP integration tests
RADIXLAB_CLI_PATH="$PWD/../cpp-core/build/radixlab-cli" mvn verify
```

The current local suites contain 84 C++ assertions, 14 CLI process checks, and
26 JUnit tests (21 unit/controller plus 5 integration). CI runs Debug and Release
C++ builds, Java tests, and the end-to-end suite on Ubuntu.

## Project layout

```text
cpp-core/
  include/radixlab/     Public C++ headers
  src/                  Parser, rational, converter implementations
  cli/                  Standalone radixlab-cli
  tests/                C++ and CLI process tests
java-backend/
  src/main/java/        Controller, service, DTOs, engine adapter
  src/main/resources/   Spring configuration and static web UI
  src/test/java/        Unit, controller, process, and integration tests
docs/                   Architecture, API, development, licenses
.github/workflows/      Three CI gates
```

## Known limitations

- One child process is started per HTTP request. This favors isolation and
  exactness over throughput; process startup adds latency and limits capacity.
- The web UI and API are intended for local or instructional deployment. There
  is no authentication, persistence, rate-limit service, or multi-instance
  coordination.
- Input and fractional output are capped at 4096 characters/digits. Very large
  exact computations are intentionally bounded at the CLI and API boundaries.
- Only ASCII digits `0-9`, `A-Z`, and `a-z` are supported; Unicode digit forms
  and numeral prefixes such as `0x` are rejected.
- The tested process-control paths target macOS and Linux.

## Roadmap

- Add an in-process C++ integration (FFI/JNI) while retaining the CLI backend.
- Add batch conversion and result caching.
- Add an OpenAPI description generated from the actual controller.
- Add property-based differential tests against an independent rational oracle.
- Add container packaging and deployment guidance once the API surface is stable.

## License

RadixLab is available under the [MIT License](LICENSE). See
[docs/THIRD_PARTY.md](docs/THIRD_PARTY.md) for dependency licenses.
