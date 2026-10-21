# HTTP API

Base URL in local development: `http://localhost:8080`

The API has one radix-conversion endpoint, seven encoding endpoints, and four
Boolean endpoints.
Static UI assets are served from `/` and `/encoding.html`.

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


## Encoding API

All encoding endpoints consume and produce `application/json`, share the process
timeout/concurrency controls described below, and reject values outside the
representable range with `422 Unprocessable Entity`.

### Signed encoding

```http
POST /api/v1/encoding/signed
```

Request:

```json
{
  "value": "-13",
  "width": 8,
  "encoding": "TWOS_COMPLEMENT",
  "negativeZero": false
}
```

| Field | Type | Required | Constraints |
| --- | --- | --- | --- |
| `value` | string | yes | decimal integer, at most 64 characters |
| `width` | integer | yes | 1–64 |
| `encoding` | string | yes | `SIGN_MAGNITUDE`, `ONES_COMPLEMENT`, or `TWOS_COMPLEMENT` (case-insensitive; `-` or `_` separators accepted) |
| `negativeZero` | boolean | no | request the distinct negative-zero pattern for value 0; sign-magnitude/ones' complement only |

Response:

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

The three encodings have different ranges:

| Encoding | 8-bit range | Negative zero |
| --- | --- | --- |
| `SIGN_MAGNITUDE` | −127 … 127 | `10000000` |
| `ONES_COMPLEMENT` | −127 … 127 | `11111111` |
| `TWOS_COMPLEMENT` | −128 … 127 | none; zero is unique |

An unrepresentable request returns `422`:

```json
{
  "timestamp": "2026-10-21T10:00:00Z",
  "status": 422,
  "error": "Unprocessable Entity",
  "message": "Value -128 is outside the representable range [-127, 127]",
  "path": "/api/v1/encoding/signed"
}
```

### Signed decoding

```http
POST /api/v1/encoding/signed/decode
```

Request:

```json
{"bits":"10000000","encoding":"SIGN_MAGNITUDE"}
```

Response:

```json
{
  "value": "0",
  "width": 8,
  "encoding": "SIGN_MAGNITUDE",
  "bits": "10000000",
  "hex": "80",
  "representable": true,
  "negativeZero": true,
  "minimum": "-127",
  "maximum": "127"
}
```

The `negativeZero` flag preserves information that a plain integer zero would
lose. Two's complement never reports it.

### BCD encoding and decoding

```http
POST /api/v1/encoding/bcd
POST /api/v1/encoding/bcd/decode
```

Encode request:

```json
{"decimal":"59"}
```

Decode request:

```json
{"bits":"01011001"}
```

Response:

```json
{
  "decimal": "59",
  "digits": "59",
  "bits": "01011001",
  "groups": ["0101", "1001"],
  "value": "59",
  "binary": "111011"
}
```

- `decimal` accepts 1–1024 digits `0-9`; leading zeros are preserved.
- BCD decode accepts 1–1024 complete 4-bit groups.
- Every nibble must be `0000`–`1001`; illegal groups such as `1010` return `400`.
- `binary` shows the ordinary positional binary representation for contrast.
- Negative BCD is intentionally not defined in this protocol.

### Gray code encoding and decoding

```http
POST /api/v1/encoding/gray
POST /api/v1/encoding/gray/decode
```

Encode request:

```json
{"value":"13","width":8}
```

Decode request:

```json
{"bits":"00001011"}
```

Response:

```json
{
  "value": "13",
  "width": 8,
  "binary": "00001101",
  "gray": "00001011",
  "binaryHex": "0D",
  "grayHex": "0B"
}
```

Gray values are unsigned and must lie in `0 … 2^width−1`. Adjacent encoded
values differ in exactly one bit. Decoding returns both the binary pattern and
the mathematical value.

### Parity

```http
POST /api/v1/encoding/parity
```

Request:

```json
{"bits":"1011001","parity":"EVEN","flipBit":3}
```

| Field | Type | Required | Constraints |
| --- | --- | --- | --- |
| `bits` | string | yes | 1–64 binary digits |
| `parity` | string | yes | `EVEN` or `ODD` (case-insensitive) |
| `flipBit` | integer | no | data bit index 0–63, less than the data width |

Response:

```json
{
  "bits": "1010001",
  "originalBits": "1011001",
  "width": 7,
  "parity": "EVEN",
  "onesCount": 4,
  "receivedOnesCount": 3,
  "parityBit": "0",
  "computedParityBit": "1",
  "dataWithParity": "10110010",
  "receivedWithParity": "10100010",
  "valid": false,
  "flipped": true,
  "flippedIndex": 3
}
```

`parityBit` is the transmitted bit generated from the original data.
`computedParityBit` is recomputed from the received data. `valid` compares them.
Parity can detect an odd number of flipped bits; it can miss an even number and
cannot locate or correct errors.


## Boolean API

Boolean requests accept expressions with variables A–Z, constants `0`/`1`,
`!`, `&`, `^`, `|`, and parentheses. Precedence is `! > & > ^ > |`; binary
operators are left-associative. Expressions are limited to 1024 characters.
Truth tables, canonical forms, and equivalence checks are limited to 6 distinct
variables. Unknown characters and syntax errors return HTTP 400 with a precise
position.

### Evaluate

```http
POST /api/v1/boolean/evaluate
```

```json
{
  "expression": "A & (!B | C)",
  "assignment": {"A": true, "B": false, "C": false}
}
```

The assignment map is optional; if omitted it is treated as empty. Every
variable used by an evaluated path must have a value. A missing variable returns
400. Assignment keys must be single uppercase letters.

Response:

```json
{
  "expression": "A & (!B | C)",
  "normalized": "(A & (!B | C))",
  "variables": ["A", "B", "C"],
  "assignment": {"A": true, "B": false, "C": false},
  "result": true,
  "ast": {
    "type": "and",
    "left": {"type": "variable", "name": "A"},
    "right": {
      "type": "or",
      "left": {"type": "not", "operand": {"type": "variable", "name": "B"}},
      "right": {"type": "variable", "name": "C"}
    }
  }
}
```

### Truth table

```http
POST /api/v1/boolean/truth-table
```

Request:

```json
{"expression":"A & B"}
```

Response:

```json
{
  "expression": "A & B",
  "normalized": "(A & B)",
  "variables": ["A", "B"],
  "rowCount": 4,
  "rows": [
    {"index":0,"assignment":{"A":false,"B":false},"result":false},
    {"index":1,"assignment":{"A":false,"B":true},"result":false},
    {"index":2,"assignment":{"A":true,"B":false},"result":false},
    {"index":3,"assignment":{"A":true,"B":true},"result":true}
  ],
  "ast": {
    "type": "and",
    "left": {"type": "variable", "name": "A"},
    "right": {"type": "variable", "name": "B"}
  }
}
```

Variables are sorted A–Z. The first variable is the most-significant bit of the
row index. A constant expression has one row and an empty assignment.

### Canonical forms

```http
POST /api/v1/boolean/canonical
```

Request:

```json
{"expression":"A ^ B"}
```

Response:

```json
{
  "expression": "A ^ B",
  "normalized": "(A ^ B)",
  "variables": ["A", "B"],
  "rowCount": 4,
  "minterms": [1, 2],
  "maxterms": [0, 3],
  "sop": "(!A & B) | (A & !B)",
  "pos": "(A | B) & (!A | !B)",
  "ast": {"type":"xor"}
}
```

Minterms are the row indices where the result is 1; maxterms are the row
indices where it is 0. These are canonical expansions, not minimized
expressions. Generated forms are reparsed and checked against the original
truth table by the engine.

### Equivalence

```http
POST /api/v1/boolean/equivalent
```

Request:

```json
{"leftExpression":"A & B","rightExpression":"A | B"}
```

Response when different:

```json
{
  "leftExpression": "A & B",
  "rightExpression": "A | B",
  "leftNormalized": "(A & B)",
  "rightNormalized": "(A | B)",
  "variables": ["A", "B"],
  "equivalent": false,
  "leftAst": {"type":"and"},
  "rightAst": {"type":"or"},
  "counterexample": {
    "assignment": {"A": false, "B": true},
    "left": false,
    "right": true
  }
}
```

The comparison uses the sorted union of both variable sets. When the expressions
are equivalent, `counterexample` is `null`. A full response always contains both
ASTs.

### Boolean parse errors

```json
{
  "timestamp": "2026-10-21T10:00:00Z",
  "status": 400,
  "error": "Bad Request",
  "message": "Left expression: Expected a variable, 0, 1, '!', or '('",
  "path": "/api/v1/boolean/equivalent",
  "code": "parse_error",
  "position": 4,
  "column": 5
}
```

`position` is zero-based; `column` is one-based. Limits such as more than six
variables return 422 with `code: "limit"`. Internal engine failures remain 502
and do not expose paths or stack traces.

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
| `400 Bad Request` | malformed JSON, missing/invalid field, invalid digit/BCD group, invalid encoding |
| `415 Unsupported Media Type` | request is not JSON |
| `404 Not Found` | unknown path |
| `405 Method Not Allowed` | wrong HTTP method |
| `422 Unprocessable Entity` | signed/Gray value outside its range, or a Boolean resource limit |
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
Output is bounded by the CLI's input limits: 4096 characters for conversion,
64 bits for signed/Gray/parity, and 4096 BCD bits.

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
