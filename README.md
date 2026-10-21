# RadixLab

[![CI](https://github.com/Asuka-bit-coder/radixlab/actions/workflows/ci.yml/badge.svg)](https://github.com/Asuka-bit-coder/radixlab/actions/workflows/ci.yml)

**Exact radix conversion and a hands-on Digital Logic Encoding Lab.**

RadixLab is an educational systems-software project with two connected purposes:

1. Convert positional numerals between bases 2 and 36 without floating-point
   approximation, including exact repeating-cycle and truncation reporting.
2. Make machine-number and digital-logic encodings experimentally visible:
   sign-magnitude, ones' complement, two's complement, BCD, reflected Gray code,
   and parity.

A C++20 engine provides arbitrary-precision arithmetic and bit-level encodings.
A Java 21 / Spring Boot service exposes the engine over HTTP and serves two
lightweight interfaces: the exact converter and the interactive Encoding Lab.

```text
101101.011₂  →  2D.6₁₆          exact, terminating
0.1₁₀        →  0.0(0011)₂      exact, repeating cycle
-13₁₀        →  11110011₂       two's complement, 8 bits
59₁₀         →  0101 1001      8421 BCD
```

## Features

### Exact radix conversion (v0.1.0)

- Bases 2 through 36, signed integers and fractions.
- Arbitrary-precision C++ integers via Boost.Multiprecision; no `double`
  intermediate representation.
- Exact rational normalization and repeating-cycle detection.
- Explicit `exact`, `repeating`, and `truncated` result flags.
- Standalone `radixlab-cli` and a REST API.

### Digital Logic Encoding Lab (v0.2.0)

- `BitVector`: fixed-width bit patterns from 1 to 64 bits, logical shifts,
  bitwise operations, and binary/hex rendering.
- Sign-magnitude (原码), ones' complement (反码), and two's complement (补码),
  including explicit negative-zero patterns.
- Strict representable ranges and `422 Unprocessable Entity` for overflow rather
  than silent truncation.
- Unsigned 8421 BCD with legal-nibble validation, preserved leading zeros, and
  ordinary-binary comparison.
- Reflected Gray code in both directions with adjacent single-bit transition
  checks.
- Even/odd parity generation, verification, and single-bit flip experiments.
- Interactive Encoding Lab with four keyboard-accessible explorers.
- Chinese course notes covering theory, algorithms, implementation, experiments,
  pitfalls, exercises, and verification.

## Exactness and encoding contracts

Radix conversion distinguishes three outcomes and never labels a truncated value
exact:

| Outcome | `exact` | `repeating` | `truncated` | Example |
| --- | --- | --- | --- | --- |
| Terminating | `true` | `false` | `false` | `0.5₁₀ = 0.1₂` |
| Complete cycle | `true` | `true` | `false` | `0.1₁₀ = 0.0(0011)₂` |
| Limit reached | `false` | `false` | `true` | `0.1₁₀ → 0.00₂` at limit 2 |

For integer encodings, the supported ranges depend on width `n`:

| Encoding | Range |
| --- | --- |
| Sign-magnitude | `-(2^(n-1)-1) … 2^(n-1)-1` |
| Ones' complement | `-(2^(n-1)-1) … 2^(n-1)-1` |
| Two's complement | `-2^(n-1) … 2^(n-1)-1` |

Sign-magnitude and ones' complement each have a negative-zero pattern. The API
reports that pattern with `negativeZero: true` instead of collapsing it into an
indistinguishable ordinary zero.

## Architecture

```text
Browser: converter + Encoding Lab
  │
  │  POST /api/v1/convert
  │  POST /api/v1/encoding/{signed|bcd|gray|parity}
  ▼
Spring Boot (Java 21)
  ├─ Controllers + Jakarta Bean Validation
  ├─ ConversionService / EncodingService
  ├─ RadixEngine          EncodingEngine
  │       └──────┬──────────────┘
  │              ▼
  │        CliEngineClient
  │          ├─ shared Semaphore
  │          ├─ shared timeout policy
  │          └─ ProcessExecutor
  │               └─ ProcessBuilder, argument vector, no shell
  ▼
radixlab-cli (C++20)
  ├─ conversion: RationalParser → RadixConverter
  └─ encoding: BitVector, SignedEncoding, BCD, Gray, Parity
```

The CLI is the process protocol between Java and C++. Every request is passed as
an argument vector, never interpolated into a shell command. The CLI writes one
JSON object to stdout and errors to stderr, with stable exit codes:

| Exit code | Meaning |
| --- | --- |
| `0` | Success |
| `1` | Internal engine failure |
| `2` | Invalid arguments or input |
| `3` | Input resource limit exceeded |
| `4` | Value outside the representable range |

See [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) for algorithms, boundaries, and
design trade-offs.

## Technology stack

| Layer | Technology |
| --- | --- |
| Exact arithmetic and encodings | C++20, Boost.Multiprecision `cpp_int` |
| C++ build/test | CMake, CTest |
| CLI | C++20, CMake |
| Backend | Java 21, Spring Boot 3.5 |
| Build/test | Maven, JUnit 5, Mockito, AssertJ |
| Web UI | Semantic HTML, CSS, vanilla JavaScript |
| CI | GitHub Actions |

## Prerequisites

- CMake 3.20 or newer.
- A C++20 compiler (Apple Clang, Clang, or GCC).
- Boost 1.70 or newer.
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

### Exact conversion CLI

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

### Encoding CLI

```bash
# -13 in 8-bit two's complement
./cpp-core/build/radixlab-cli encoding \
  --operation signed-encode --value -13 --width 8 \
  --encoding TWOS_COMPLEMENT --json
# {"operation":"signed-encode","value":"-13","width":8,
#  "encoding":"TWOS_COMPLEMENT","bits":"11110011","hex":"F3", ...}

# 8421 BCD
./cpp-core/build/radixlab-cli encoding \
  --operation bcd-encode --decimal 59 --json
# ... "bits":"01011001", "groups":["0101","1001"], ...

# Reflected Gray code
./cpp-core/build/radixlab-cli encoding \
  --operation gray-encode --value 13 --width 8 --json
# ... "binary":"00001101", "gray":"00001011", ...

# Flip data bit 3 and observe parity failure
./cpp-core/build/radixlab-cli encoding \
  --operation parity --bits 1011001 --parity EVEN --flip-bit 3 --json
# ... "valid":false, "computedParityBit":"1", ...
```

Run `./cpp-core/build/radixlab-cli encoding --help` for all operations and
options.

## Run the Java backend and web UI

Build the CLI first, then start Spring Boot:

```bash
cd java-backend
RADIXLAB_ENGINE_CLI_PATH=../cpp-core/build/radixlab-cli \
  mvn spring-boot:run
```

Open <http://localhost:8080> for exact conversion and
<http://localhost:8080/encoding.html> for the Encoding Lab. To run the packaged
jar instead:

```bash
cd java-backend
mvn package
RADIXLAB_ENGINE_CLI_PATH=../cpp-core/build/radixlab-cli \
  java -jar target/radixlab-backend-0.2.0.jar
```

Configuration:

| Property | Environment variable | Default |
| --- | --- | --- |
| `radixlab.engine.cli-path` | `RADIXLAB_ENGINE_CLI_PATH` | `radixlab-cli` on `PATH` |
| `radixlab.engine.timeout` | `RADIXLAB_ENGINE_TIMEOUT` | `5s` |
| `radixlab.engine.max-concurrent` | `RADIXLAB_ENGINE_MAX_CONCURRENT` | `4` |

## API

The API has one conversion endpoint and seven encoding endpoints:

```text
POST /api/v1/convert
POST /api/v1/encoding/signed          encode value → bits
POST /api/v1/encoding/signed/decode   decode bits → value
POST /api/v1/encoding/bcd             encode decimal → BCD
POST /api/v1/encoding/bcd/decode      decode BCD → decimal
POST /api/v1/encoding/gray            encode value → Gray code
POST /api/v1/encoding/gray/decode     decode Gray code → value
POST /api/v1/encoding/parity          generate/verify and optionally flip one bit
```

### Conversion

```bash
curl -sS http://localhost:8080/api/v1/convert \
  -H 'Content-Type: application/json' \
  -d '{"number":"101101.011","fromBase":2,"toBase":16}'
```

```json
{
  "result": "2D.6",
  "exact": true,
  "truncated": false,
  "repeating": false
}
```

### Signed encoding

```bash
curl -sS http://localhost:8080/api/v1/encoding/signed \
  -H 'Content-Type: application/json' \
  -d '{"value":"-13","width":8,"encoding":"TWOS_COMPLEMENT"}'
```

```json
{
  "value": "-13",
  "width": 8,
  "encoding": "TWOS_COMPLEMENT",
  "bits": "11110011",
  "hex": "F3",
  "representable": true,
  "negativeZero": false,
  "minimum": "-128",
  "maximum": "127"
}
```

An out-of-range value returns `422 Unprocessable Entity`; it is never silently
truncated. Negative zero can be requested when encoding value `0` in
sign-magnitude or ones' complement, and is reported when decoding such a pattern.

### BCD, Gray, and parity

```bash
curl -sS http://localhost:8080/api/v1/encoding/bcd \
  -H 'Content-Type: application/json' \
  -d '{"decimal":"59"}'

curl -sS http://localhost:8080/api/v1/encoding/gray \
  -H 'Content-Type: application/json' \
  -d '{"value":"13","width":8}'

curl -sS http://localhost:8080/api/v1/encoding/parity \
  -H 'Content-Type: application/json' \
  -d '{"bits":"1011001","parity":"EVEN","flipBit":3}'
```

BCD responses include the per-digit groups and ordinary binary for comparison.
Gray responses include both binary and Gray patterns. Parity responses include
the original/received bits, generated/recomputed parity bits, validity, and the
optional flipped index.

Errors use a stable shape:

```json
{
  "timestamp": "2026-10-21T10:00:00Z",
  "status": 400,
  "error": "Bad Request",
  "message": "width must be between 1 and 64",
  "path": "/api/v1/encoding/signed"
}
```

HTTP statuses include `400` for invalid requests, `422` for unrepresentable
values, `502` for engine failure, `503` when all process slots are busy, and
`504` for a timeout. Full semantics are in [docs/API.md](docs/API.md).

## Course materials

The digital-logic course companion is in [docs/course/](docs/course/):

- [Overview and lab workflow](docs/course/00-overview.md)
- [Number systems and fixed-width patterns](docs/course/01-number-systems.md)
- [Signed representations](docs/course/02-signed-representation.md)
- [8421 BCD](docs/course/03-bcd.md)
- [Reflected Gray code](docs/course/04-gray-code.md)
- [Parity](docs/course/05-parity.md)
- [Exercises and verification](docs/course/exercises.md)

Each topic follows **Theory → Algorithm → Implementation → Experiment →
Verification**, with boundary cases, pitfalls, API/CLI examples, and exercises.

## Test everything

```bash
# C++ unit, encoding, and CLI process tests
ctest --test-dir cpp-core/build --output-on-failure

# Java unit and controller tests
cd java-backend
mvn test

# Java unit tests plus real Java → C++ HTTP integration tests
RADIXLAB_CLI_PATH="$PWD/../cpp-core/build/radixlab-cli" mvn verify
```

The current local suites contain 258 C++ assertions, 25 CLI process checks, and
55 JUnit tests (39 unit/controller plus 16 integration). CI runs Debug and
Release C++ builds, Java tests, and the full end-to-end suite on Ubuntu.

## Project layout

```text
cpp-core/
  include/radixlab/encoding/   BitVector and encoding headers
  src/encoding/                Signed, BCD, Gray, parity implementations
  cli/                         Conversion + encoding CLI
  tests/                       C++ and CLI process tests
java-backend/
  src/main/java/               Controllers, services, DTOs, engine adapters
  src/main/resources/static/   Converter and Encoding Lab UI
  src/test/java/               Unit, controller, process, integration tests
docs/
  course/                      Chinese digital-logic course companion
  API.md ARCHITECTURE.md DEVELOPMENT.md THIRD_PARTY.md
.github/workflows/             CI gates
```

## Development

AI-assisted development tools, including Claude Code, have been used in this
project. Project ownership and release decisions remain with the repository
maintainer.

## Known limitations

- One child process is started per HTTP request. This favors isolation and
  exactness over throughput; process startup adds latency and limits capacity.
- The API and web UI are intended for local or instructional deployment. There
  is no authentication, persistence, rate-limit service, or multi-instance
  coordination.
- Radix conversion input and fractional output are capped at 4096
  characters/digits. Encoding widths are 1–64 bits; BCD accepts up to 1024
  decimal digits.
- Only ASCII digit characters and the documented encoding names are supported.
  Unicode digit forms and prefix syntax such as `0x` are rejected.
- BCD in this release is explicitly unsigned. No signed BCD, packed-decimal
  arithmetic, Hamming code, or error correction is implemented.
- Parity can detect an odd number of bit errors but can miss an even number and
  cannot locate or correct errors.
- The tested process-control paths target macOS and Linux.

## Roadmap

- Boolean algebra, truth tables, Karnaugh maps, and combinational logic.
- Hamming code and error-correction experiments.
- Signed BCD and decimal arithmetic protocols.
- An in-process C++ integration (FFI/JNI) while retaining the CLI backend.
- An OpenAPI description generated from the actual controllers.
- Property-based differential tests against independent arithmetic oracles.

## License

RadixLab is available under the [MIT License](LICENSE). See
[docs/THIRD_PARTY.md](docs/THIRD_PARTY.md) for dependency licenses.
