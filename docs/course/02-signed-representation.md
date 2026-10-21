# 02 原码、反码与补码

## 1. 学习目标

完成本章后，你应能：

- 在固定位宽下写出 Sign-Magnitude、Ones' Complement、Two's Complement；
- 从位模式反解数值；
- 说明三种编码的范围差异；
- 正确处理负零；
- 解释为什么补码的最小值比原码/反码低 1；
- 使用 RadixLab 的 API、CLI 与 Signed Explorer 验证结论。

## 2. 理论：三种解释规则

设位宽为 `n`，最高位 `b[n-1]` 为符号位。

### 2.1 原码 Sign-Magnitude

最高位只表示符号：

- `0`：非负；
- `1`：负。

低 `n-1` 位表示绝对值。

\[
x =
\begin{cases}
+\sum_{i=0}^{n-2} b_i 2^i, & b_{n-1}=0 \\
-\sum_{i=0}^{n-2} b_i 2^i, & b_{n-1}=1
\end{cases}
\]

范围：

\[
-(2^{n-1}-1) \le x \le 2^{n-1}-1
\]

### 2.2 反码 Ones' Complement

正数与原码相同。负数把绝对值的全部 `n` 位逐位取反（包括符号位）。

若 `M = 2^n - 1`，负数 `x < 0` 的模式为：

\[
P = M - |x|
\]

也可以写成 `P = 0xFFFF... ^ |x|`（在 n 位内）。

范围与原码相同：

\[
-(2^{n-1}-1) \le x \le 2^{n-1}-1
\]

### 2.3 补码 Two's Complement

正数相同。负数模式为：

\[
P = 2^n - |x|
\]

或等价地“反码加一”。

范围：

\[
-2^{n-1} \le x \le 2^{n-1}-1
\]

### 2.4 负零

原码与反码各有两种零的位模式：

| 编码 | +0 | −0 | 解码数值 |
| --- | --- | --- | --- |
| 8 位原码 | `00000000` | `10000000` | 0 |
| 8 位反码 | `00000000` | `11111111` | 0 |
| 8 位补码 | `00000000` | 无独立模式 | 0 |

负零不是一个“更小的零”。它的数值仍是 0，但位模式与正零不同。RadixLab 用
`negativeZero` 字段保留这一区别。补码的零唯一，因此请求补码负零会被拒绝。

## 3. 经典 8 位案例

| 十进制 | 原码 | 反码 | 补码 |
| --- | --- | --- | --- |
| +13 | `00001101` | `00001101` | `00001101` |
| −13 | `10001101` | `11110010` | `11110011` |
| +127 | `01111111` | `01111111` | `01111111` |
| −127 | `11111111` | `10000000` | `10000001` |
| −128 | 不可表示 | 不可表示 | `10000000` |
| 0 | `00000000` | `00000000` | `00000000` |
| 负零 | `10000000` | `11111111` | 不适用 |

观察：

- −127 的原码是 `11111111`，这与反码的负零模式相同，但编码形式不同，数值解释也不同。
- 补码中 `10000000` 表示 −128，而不是负零。
- 原码/反码的最小值都是 −127；补码能多表示一个负数 −128。

## 4. 算法

### 4.1 编码前检查范围

对于位宽 `n`：

```text
max = 2^(n-1) - 1
min_sm_ones = -max
min_twos = -2^(n-1)
```

输入超出范围时必须返回范围错误，不能截断。

### 4.2 原码编码

```text
if negativeZero and value == 0:
    pattern = 1 << (n-1)
else if value < 0:
    pattern = (1 << (n-1)) | (-value)
else:
    pattern = value
```

### 4.3 反码编码

```text
if negativeZero and value == 0:
    pattern = (1 << n) - 1
else if value < 0:
    pattern = ((1 << n) - 1) ^ (-value)
else:
    pattern = value
```

### 4.4 补码编码

```text
if value < 0:
    pattern = (1 << n) + value
else:
    pattern = value
```

`1 << n` 使用任意精度 `cpp_int`，避免 64 位边界上的未定义行为。

### 4.5 解码

```text
sign = bit(n-1)

Sign-Magnitude:
    magnitude = low n-1 bits
    value = sign ? -magnitude : magnitude
    negativeZero = sign && magnitude == 0

Ones' Complement:
    value = sign ? -(((1 << n) - 1) ^ pattern) : pattern
    negativeZero = sign && complement == 0

Two's Complement:
    value = sign ? pattern - (1 << n) : pattern
```

## 5. C++ 实现位置

| 内容 | 文件 |
| --- | --- |
| 编码枚举、范围、结果结构 | `cpp-core/include/radixlab/encoding/signed_encoding.hpp` |
| 编码/解码算法 | `cpp-core/src/encoding/signed_encoding.cpp` |
| 范围异常 | `cpp-core/include/radixlab/encoding/encoding_error.hpp` |
| 测试 | `cpp-core/tests/test_signed_encoding.cpp` |

关键接口：

```cpp
SignedEncodeResult encodeSigned(
    BigInt value, std::size_t width,
    SignedEncoding encoding, bool negativeZero = false);

SignedDecodeResult decodeSigned(
    const BitVector& bits, SignedEncoding encoding);
```

## 6. API 与 CLI 实验

### 6.1 编码 −13（8 位补码）

```bash
./cpp-core/build/radixlab-cli encoding \
  --operation signed-encode --value -13 --width 8 \
  --encoding TWOS_COMPLEMENT --json
```

关键输出：

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

### 6.2 解码负零（原码）

```bash
./cpp-core/build/radixlab-cli encoding \
  --operation signed-decode --bits 10000000 \
  --encoding SIGN_MAGNITUDE --json
```

返回的 `value` 是 `"0"`，`negativeZero` 是 `true`。如果只返回数值 0，就丢失了
编码中保留的负零信息。

### 6.3 请求负零（反码）

```bash
./cpp-core/build/radixlab-cli encoding \
  --operation signed-encode --value 0 --width 8 \
  --encoding ONES_COMPLEMENT --negative-zero --json
```

位模式应为 `11111111`。

### 6.4 REST API

```bash
curl -sS http://localhost:8080/api/v1/encoding/signed \
  -H 'Content-Type: application/json' \
  -d '{"value":"-13","width":8,"encoding":"TWOS_COMPLEMENT"}'
```

解码：

```bash
curl -sS http://localhost:8080/api/v1/encoding/signed/decode \
  -H 'Content-Type: application/json' \
  -d '{"bits":"10000000","encoding":"SIGN_MAGNITUDE"}'
```

### 6.5 范围错误

```bash
curl -i http://localhost:8080/api/v1/encoding/signed \
  -H 'Content-Type: application/json' \
  -d '{"value":"-128","width":8,"encoding":"SIGN_MAGNITUDE"}'
```

应返回 `422 Unprocessable Entity`，而不是 200 或静默得到某个 8 位模式。

## 7. 网页实验：Signed Explorer

1. 打开 <http://localhost:8080/encoding.html#signed>。
2. 选择 8 位、补码，输入 −13。
3. 观察最左侧高亮格子的符号位、`11110011`、`0xF3` 和范围 `-128 … 127`。
4. 切换到原码，输入 −13，观察位模式变为 `10001101`。
5. 输入 0，勾选 “Encode negative zero”（仅原码/反码可用），观察负零提示。
6. 切换 “Bits → value”，分别用原码和补码解析 `10000000`，观察负零与 −128 的差异。

位格下方的小数字是 bit index；最左格是符号位，通常索引为 `width−1`。

## 8. 常见误区

- **把原码 −127 的 `11111111` 当成补码 −1**：同一个位模式必须结合编码形式解释。
- **认为补码和原码只是最高位不同**：只有正数相同，负数规则不同。
- **将 −128 用原码强行写成 `10000000`**：这是原码负零，不是 −128。
- **忽略范围检查**：超出范围时截断高位会产生另一个合法位模式，但不是原值的编码。
- **把负零当作特殊数学值**：数值仍为 0；区别只在位模式与解释是否保留符号零。
- **在 64 位边界使用有符号移位**：项目使用 `cpp_int`；手写 C/C++ 时应使用无符号类型并明确掩码。

## 9. 练习

1. 写出 4 位下 −6 的原码、反码、补码。
2. 为什么 4 位原码不能表示 −8？
3. 位模式 `1111` 在 4 位原码、反码、补码下分别是什么数值？
4. 写出 16 位下 −1 的反码和补码（十六进制）。
5. 解释为什么补码的负零请求应当被拒绝。

### 答案与验证

1. 4 位：原码 `1110`（符号 1 + 绝对值 110）；反码 `1001`（`1111 ^ 0110`）；
   补码 `1010`（16 − 6）。
2. 4 位原码的数值范围是 −7 … 7，绝对值部分只有 3 位，最大 7；−8 超出范围。
3. 原码：符号位 1，绝对值 111 = 7，所以是 −7；反码：`1111 ^ 1111 = 0000`，
   是负零（数值 0）；补码：15 − 16 = −1。
4. 16 位 −1 的反码：`FFFF ^ 0001 = FFFE`；补码：`10000 − 1 = FFFF`。
5. 补码只有一个零模式 `000...0`。如果发明第二个负零模式，会与“补码按模 2^n
   解释”的一一对应关系冲突，并破坏加法器的唯一零表示。RadixLab 因此拒绝该请求。

验证：

```bash
./cpp-core/build/radixlab-cli encoding \
  --operation signed-encode --value -6 --width 4 \
  --encoding TWOS_COMPLEMENT --json
```

下一章进入 BCD：一种按十进制位编码、与普通二进制不同的表示。
