# Development Guide

## Toolchain

| Tool | Version used | Notes |
| --- | --- | --- |
| C++ compiler | Apple Clang 21 / GCC 13+ | C++20 required |
| CMake | 4.4.4 locally; 3.20 minimum | Prefers `BoostConfig.cmake`; header-only fallback supports CMake 4 |
| Boost | 1.92 locally; 1.70 minimum | Headers and CMake config required |
| Java | Temurin 21.0.12 | `java.version` is 21 |
| Maven | 3.10 locally; 3.9+ recommended | Spring Boot 3.5.16 parent |
| OS | macOS 26 / Ubuntu 24.04 CI | Process tests use POSIX `sh` and `sleep` |

Install dependencies on macOS:

```bash
brew install boost cmake maven
```

On Debian/Ubuntu:

```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake libboost-dev maven
```

Use JDK 21 for Maven. If multiple JDKs are installed:

```bash
export JAVA_HOME=$(/usr/libexec/java_home -v 21)   # macOS
# export JAVA_HOME=/usr/lib/jvm/temurin-21-jdk      # Linux example
```

## Repository hygiene

Build output is ignored. Do not commit:

- `cpp-core/build/`
- `java-backend/target/`
- `.env` files, tokens, local paths, logs, IDE metadata, or `.claude/` local settings.

The C++ library has no generated source. The Java jar is reproducible from the
Maven project and is not committed.

## C++ workflow

Configure once and rebuild incrementally:

```bash
cmake -S cpp-core -B cpp-core/build -DCMAKE_BUILD_TYPE=Debug
cmake --build cpp-core/build --parallel
ctest --test-dir cpp-core/build --output-on-failure
```

Release verification:

```bash
cmake -S cpp-core -B cpp-core/build-release -DCMAKE_BUILD_TYPE=Release
cmake --build cpp-core/build-release --parallel
ctest --test-dir cpp-core/build-release --output-on-failure
```

Tests are standalone executables that return non-zero on failure. They do not
rely on `assert()` semantics, so both build types exercise the same checks.
`BUILD_TESTING` is provided by CTest and defaults to `ON`.

Run a subset:

```bash
ctest --test-dir cpp-core/build -R RadixConverterTests --output-on-failure
ctest --test-dir cpp-core/build -R 'BitVectorTests|SignedEncodingTests|BcdTests' --output-on-failure
ctest --test-dir cpp-core/build -R CliTests --output-on-failure
```

The CLI test is a CMake script (`cpp-core/tests/test_cli.cmake`) that launches
the built executable and checks stdout, stderr, exit codes, JSON, and limits.

### Adding C++ behavior

1. Put reusable digit logic in `include/radixlab/digits.hpp`.
2. Preserve `Rational` invariants at construction rather than at call sites.
3. Keep conversion and encoding in integer arithmetic; do not introduce floating point.
4. Keep `BitVector` as a pattern, not an implicit signed value.
5. Preserve representable-range checks; never mask an out-of-range input.
6. Extend the executable tests for each success and failure state.
7. Add a CLI process case if the public process contract changes.
8. Build with `-Wall -Wextra -Wpedantic -Wconversion` and remove new warnings.

## Java workflow

```bash
cd java-backend
mvn test        # unit, controller, adapter, process tests
mvn package     # builds target/radixlab-backend-0.2.0.jar
```

`mvn test` excludes tests tagged `integration`. Full verification requires the
C++ binary:

```bash
cd java-backend
RADIXLAB_CLI_PATH="$PWD/../cpp-core/build/radixlab-cli" mvn verify
```

Failsafe runs classes matching `**/*IntegrationTest.java` with the `integration`
tag. The integration test fails, rather than skips, when the CLI is missing. This
prevents a false-green CI run.

Run a single test:

```bash
mvn -Dtest=CliRadixEngineTest test
```

### Manual backend smoke test

Conversion and encoding share the same CLI path and process timeout.

Terminal 1:

```bash
cd java-backend
RADIXLAB_ENGINE_CLI_PATH="$PWD/../cpp-core/build/radixlab-cli" \
  mvn spring-boot:run
```

Terminal 2:

```bash
curl -sS http://localhost:8080/api/v1/convert \
  -H 'Content-Type: application/json' \
  -d '{"number":"0.1","fromBase":10,"toBase":2}'
```

Expected:

```json
{"result":"0.0(0011)","exact":true,"truncated":false,"repeating":true}
```

Open <http://localhost:8080> for the converter and
<http://localhost:8080/encoding.html> for the Encoding Lab.

Encoding smoke test:

```bash
curl -sS http://localhost:8080/api/v1/encoding/signed \
  -H 'Content-Type: application/json' \
  -d '{"value":"-13","width":8,"encoding":"TWOS_COMPLEMENT"}'
```

Expected:

```json
{"value":"-13","width":8,"encoding":"TWOS_COMPLEMENT","bits":"11110011","hex":"F3","representable":true,"negativeZero":false,"minimum":"-128","maximum":"127"}
```

### Configuration

Spring binds these properties, and relaxed binding accepts the environment
variables shown in the README:

```properties
radixlab.engine.cli-path=radixlab-cli
radixlab.engine.timeout=5s
radixlab.engine.max-concurrent=4
```

For local development, point `cli-path` at the built binary. A bare command name
requires the binary to be on `PATH`.

### Adding backend behavior

- Validate untrusted request data at the DTO boundary.
- Keep OS process concerns behind `ProcessExecutor` and shared limits behind `CliEngineClient`.
- Keep conversion semantics behind `RadixEngine`; keep encoding semantics behind `EncodingEngine`.
- Map input failures to 400, representable-range failures to 422, and engine failures to 502/503/504.
- Do not leak engine stderr or paths.
- Add a unit/controller test and, when the real CLI contract is involved, an
  integration test.
- Never concatenate request fields into a shell command.

## CI gates

`.github/workflows/ci.yml` has three independent jobs:

1. **C++ / Debug and Release** — compile and run CTest on Ubuntu.
2. **Java unit tests** — Maven unit, controller, adapter, and process tests.
3. **End-to-end integration** — build the real CLI, then run `mvn verify` so the
   HTTP integration suite invokes C++.

The workflow installs Boost with `apt`, sets up Temurin 21, and uses no
`continue-on-error` or ignored failures.

## Troubleshooting

### `Could not find a package configuration file provided by "Boost"`

Install the Boost development package. RadixLab prefers `find_package(Boost CONFIG)`
for CMake 4 compatibility, then falls back to locating the header-only include
tree directly if no Boost CMake package is installed.

### `Integration test requires the C++ CLI`

Build `cpp-core` first or set `RADIXLAB_CLI_PATH` to an absolute path.

### `Value ... outside the representable range` with HTTP 422

The request is valid but cannot be encoded at that width. Choose a larger width or
an encoding with a wider range. Two's complement is one value wider than the
other forms.

### `Conversion engine failed` with HTTP 502

Check the backend log. Common causes are a wrong CLI path, a non-executable file,
or an engine crash. The client intentionally receives no internal details.

### Maven runs a JDK newer than 21

Set `JAVA_HOME` to a JDK 21 installation, as shown above.

### Port 8080 is already in use

```bash
mvn spring-boot:run -Dspring-boot.run.arguments=--server.port=8081
```

For the jar, append `--server.port=8081`.

## Release checklist

1. C++ Debug and Release tests pass.
2. `mvn verify` passes with the real CLI.
3. Encoding endpoints and both UI pages pass the integration suite.
4. CI jobs pass on the pushed commit.
5. README, API, course notes, and observed outputs agree.
6. No secrets or build artifacts are staged.
7. Commit authorship and repository metadata are reviewed before release.
8. Version strings agree across CMake, CLI, `pom.xml`, and release tag.
