# HTTP API

Base URL in local development: `http://localhost:8080`

There is one public conversion endpoint. Static UI assets are served from `/`.

## Convert a numeral

```http
POST /api/v1/convert
Content-Type: application/json
```

### Request body

```json
{
  "number": "101101.011",
  "fromBase": 2,
  "toBase": 16,
  "maxFractionDigits": 256
}
```

| Field | Type | Required | Constraints |
| --- | --- | --- | --- |
| `number` | string | yes | 1–4096 characters; optional `+`/`-`; at most one `.`; at least one ASCII digit |
| `fromBase` | integer | yes | 2–36 |
| `toBase` | integer | yes | 2–36 |
| `maxFractionDigits` | integer | no | 0–4096; default 256 |

Digits are `0-9` and `A-Z`/`a-z`. A digit must be less than `fromBase`.
Whitespace and prefixes such as `0x` are not accepted. Fractions may appear on
either side of the radix point: `.5` and `5.` are valid.

### Success response

`200 OK`

```json
{
  "result": "2D.6",
  "exact": true,
  "truncated": false,
  "repeating": false
}
```

| Field | Meaning |
| --- | --- |
| `result` | Converted numeral, uppercase; cycles use `prefix(cycle)` |
| `exact` | True when the result represents the complete value |
| `truncated` | True when `maxFractionDigits` stopped output early |
| `repeating` | True when a complete fractional cycle was detected |

`exact=true` with `repeating=true` is a complete repeating value, not an
approximation. `truncated=true` always implies `exact=false`.

### Repeating and truncated examples

Request:

```json
{"number":"0.1","fromBase":10,"toBase":2}
```

Response:

```json
{
  "result": "0.0(0011)",
  "exact": true,
  "truncated": false,
  "repeating": true
}
```

Request:

```json
{"number":"0.1","fromBase":10,"toBase":2,"maxFractionDigits":2}
```

Response:

```json
{
  "result": "0.00",
  "exact": false,
  "truncated": true,
  "repeating": false
}
```

### Arbitrary precision

The engine is not limited to 64-bit values.

```bash
curl -sS http://localhost:8080/api/v1/convert \
  -H 'Content-Type: application/json' \
  -d '{"number":"18446744073709551616","fromBase":10,"toBase":16}'
```

```json
{
  "result": "10000000000000000",
  "exact": true,
  "truncated": false,
  "repeating": false
}
```

## Errors

Error responses use a consistent object:

```json
{
  "timestamp": "2026-10-21T10:00:00Z",
  "status": 400,
  "error": "Bad Request",
  "message": "fromBase must be between 2 and 36",
  "path": "/api/v1/convert"
}
```

| Status | Condition |
| --- | --- |
| `400 Bad Request` | malformed JSON, missing/invalid field, invalid digit for base |
| `404 Not Found` | unknown path |
| `405 Method Not Allowed` | wrong HTTP method |
| `500 Internal Server Error` | unexpected server error |
| `502 Bad Gateway` | CLI cannot start, exits abnormally, or returns unreadable output |
| `503 Service Unavailable` | all configured process slots are busy |
| `504 Gateway Timeout` | CLI exceeded `radixlab.engine.timeout` |

Internal paths, stack traces, and engine stderr are not returned. The `message`
is a stable client-facing description.

## Capacity and timeout behavior

The backend limits concurrent CLI processes with
`radixlab.engine.max-concurrent` (default 4). A request waits up to 500 ms for a
slot. If none is available, it receives `503`; clients may retry with backoff.

Each child process has `radixlab.engine.timeout` (default 5 seconds). On expiry
the process is destroyed, then forcibly terminated if needed. The request
receives `504`.

The JVM reads stdout and stderr concurrently, preventing output-pipe deadlocks.
Output is bounded by the CLI's 4096-character input and fractional limits.

## Example session

```bash
# Start the backend first (see README.md).
curl -i http://localhost:8080/api/v1/convert \
  -H 'Content-Type: application/json' \
  -d '{
    "number": "-101101.011",
    "fromBase": 2,
    "toBase": 8
  }'
```

```http
HTTP/1.1 200 OK
Content-Type: application/json

{"result":"-55.3","exact":true,"truncated":false,"repeating":false}
```

## Operational limitations

- No authentication or authorization.
- No request rate limiting beyond process-slot backpressure.
- No persistence or history.
- Designed for a single instance; process slots are local to the JVM.
- One process per request; latency includes engine startup.
