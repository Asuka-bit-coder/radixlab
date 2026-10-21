# 03 8421 BCD

## 1. 学习目标

完成本章后，你应能：

- 说明 BCD 与普通二进制的本质区别；
- 把十进制数字串编码为 8421 BCD；
- 检查非法 BCD 四位组；
- 解释前导零在 BCD 中的处理；
- 计算 BCD 位数与十进制位数的关系；
- 用 CLI、API 和 BCD Explorer 验证编解码。

## 2. 理论：逐位十进制编码

BCD（Binary-Coded Decimal）把每个十进制数字独立编码为 4 位。8421 指四位权值：

\[
8b_3 + 4b_2 + 2b_1 + b_0
\]

合法码字只有 10 个：

| 十进制 | 8421 BCD |
| --- | --- |
| 0 | `0000` |
| 1 | `0001` |
| 2 | `0010` |
| 3 | `0011` |
| 4 | `0100` |
| 5 | `0101` |
| 6 | `0110` |
| 7 | `0111` |
| 8 | `1000` |
| 9 | `1001` |

`1010` 到 `1111` 六个四位组在 8421 BCD 中没有合法十进制含义，必须拒绝。

### 2.1 位数关系

一个 `d` 位十进制数需要：

\[
4d
\]

个 BCD 位。它不是二进制下所需的 `ceil(log2(10^d))` 位。

例如：

- 59 的普通二进制是 `111011`，只需 6 位；
- 59 的 BCD 是 `0101 1001`，需要 8 位。

BCD 牺牲存储密度，换取十进制显示和逐位硬件处理的便利。

### 2.2 前导零

本项目明确保留十进制输入的前导零：

| 输入 | BCD | 解码后的 digits |
| --- | --- | --- |
| `59` | `0101 1001` | `59` |
| `0059` | `0000 0000 0101 1001` | `0059` |
| `0` | `0000` | `0` |

前导零不改变数学数值，但改变 BCD 的组数和固定宽度位模式。这是教学与传输场景
中有意义的区别。

### 2.3 符号

本版本的 BCD 协议只接受无符号十进制数字串。负数、正号、小数点、空格和十六
进制字母都不是合法输入。不要自行把负数套入 8421 BCD 而没有明确规范。

## 3. 算法

### 3.1 编码

```text
for each decimal digit d:
    q = decimal_value(d)          // 0..9
    nibble = binary(q, 4 bits)    // 0000..1001
    append nibble
```

同时可以用 `value = value * 10 + d` 计算普通数值，以便输出普通二进制对照。

### 3.2 解码

```text
1. 去掉用于阅读的空格
2. 检查长度大于 0 且是 4 的倍数
3. 每 4 位取一个 nibble
4. 若 nibble 的数值大于 9，拒绝
5. 将合法 nibble 转成十进制字符，并保留组顺序
```

### 3.3 非法码字

`1010` 的位权计算是 10，但十进制数字只能是 0–9，因此非法。类似的
`1111 = 15` 也非法。不能把它“解释成 A 或 F”，除非进入十六进制而不是 BCD。

## 4. C++ 实现位置

| 内容 | 文件 |
| --- | --- |
| 接口与结果结构 | `cpp-core/include/radixlab/encoding/bcd.hpp` |
| 编解码和合法 nibble 检查 | `cpp-core/src/encoding/bcd.cpp` |
| 测试 | `cpp-core/tests/test_bcd.cpp` |

结果结构同时给出：

- `digits`：保留前导零的十进制字符；
- `bits`：连续的 BCD 位串；
- `groups`：每四位一组，便于显示；
- `value`：数学数值的规范十进制字符串；
- `binary`：普通二进制表示，用于对比。

## 5. API 与 CLI 实验

### 5.1 编码 59

```bash
./cpp-core/build/radixlab-cli encoding \
  --operation bcd-encode --decimal 59 --json
```

关键字段：

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

### 5.2 编码前导零

```bash
./cpp-core/build/radixlab-cli encoding \
  --operation bcd-encode --decimal 0059 --json
```

`bits` 为 `0000000001011001`，`digits` 仍为 `0059`。

### 5.3 解码

```bash
./cpp-core/build/radixlab-cli encoding \
  --operation bcd-decode --bits "0101 1001" --json
```

CLI 接受空格以便阅读，但 API 的 `bits` 字段要求连续的四位组串，例如
`01011001`。

### 5.4 非法码字

```bash
./cpp-core/build/radixlab-cli encoding \
  --operation bcd-decode --bits 1010 --json
# exit code 2, error code invalid_input
```

REST API 对 `{"bits":"1010"}` 返回 `400 Bad Request`，消息包含
“Illegal BCD nibble”。

### 5.5 REST API

```bash
curl -sS http://localhost:8080/api/v1/encoding/bcd \
  -H 'Content-Type: application/json' \
  -d '{"decimal":"59"}'
```

## 6. 网页实验：BCD Explorer

1. 打开 <http://localhost:8080/encoding.html#bcd>。
2. 输入 `59`，观察两个数字卡片分别显示 5→`0101`、9→`1001`。
3. 比较 “Packed BCD” 与 “Ordinary binary”：8 位对 6 位。
4. 输入 `0059`，观察四个 BCD 组和前导零。
5. 输入 `10A` 或负号，观察输入错误。API/CLI 拒绝非十进制数字。

## 7. 常见误区

- **把 BCD 当成“以 10 为基数的二进制”**：BCD 是逐位编码，不是普通位置二进制。
- **把 `01011001` 直接按二进制读成 89**：它在 BCD 中表示 59。
- **忽略非法 nibble**：`1010`–`1111` 不是合法 8421 BCD。
- **丢失前导零**：对数学值无影响，但会改变 BCD 位模式和组数。
- **给 BCD 随意加负号**：本协议没有定义负数 BCD。
- **把十六进制 A–F 当作 BCD 数字**：十六进制和 BCD 是不同编码。

## 8. 练习

1. 将十进制 27 编码为 8421 BCD，并与普通二进制比较。
2. `10000110` 是合法 BCD 吗？若合法，表示多少？
3. 为什么 999 的 BCD 需要 12 位，而普通二进制只需 10 位？
4. 输入 `0007` 与 `7` 的 BCD 有什么不同？
5. 解释为什么 `1100` 不能解释为 BCD 数字。

### 答案与验证

1. `27 → 0010 0111`，8 位；`27₁₀ = 11011₂`，5 位。
2. 分为 `1000 0110`，两个 nibble 分别是 8 和 6，合法，表示 86。
3. 999 有 3 个十进制位，每位 4 位，共 12 位；普通二进制
   `999 = 1111100111₂`，10 位。
4. `0007` 有 4 个 BCD 组，位串 16 位，`digits` 保留 `0007`；`7` 只有 1 组
   4 位。两者数学值都是 7。
5. `1100 = 12`，超出十进制数字 0–9；8421 BCD 不存在“12”这个单独数字。

验证：

```bash
./cpp-core/build/radixlab-cli encoding \
  --operation bcd-decode --bits 10000110 --json
```

下一章讨论 Gray Code：它关心的不是十进制值，而是相邻位置只改变一位。
