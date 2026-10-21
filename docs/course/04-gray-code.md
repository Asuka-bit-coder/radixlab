# 04 反射 Gray Code

## 1. 学习目标

完成本章后，你应能：

- 用 `G = B xor (B >> 1)` 把二进制转为 Gray Code；
- 从 Gray Code 恢复二进制与无符号数值；
- 证明并验证相邻 Gray 码只差一位；
- 解释 Gray Code 为什么不是普通进制；
- 使用 CLI、API 和 Gray Explorer 观察相邻切换。

## 2. 理论：变化最小的编码

Gray Code 是一种无符号的固定位宽编码。它最重要的性质是：

> 相邻的两个编码值只改变一个二进制位。

反射 Gray Code 的构造与普通二进制 `B` 的关系是：

\[
G = B \oplus (B \gg 1)
\]

其中 `>>` 是逻辑右移，空位补 0，`⊕` 是逐位异或。

反向转换：

- 最高位相同：`b[n-1] = g[n-1]`；
- 从高到低：`b[i] = b[i+1] xor g[i]`；
- 最终二进制数值按通常的位置权解释。

Gray Code 的取值仍是 `0 … 2^n−1`，但码字顺序不是普通二进制顺序。

### 2.1 4 位序列

| 无符号值 | 二进制 | Gray |
| --- | --- | --- |
| 0 | `0000` | `0000` |
| 1 | `0001` | `0001` |
| 2 | `0010` | `0011` |
| 3 | `0011` | `0010` |
| 4 | `0100` | `0110` |
| 5 | `0101` | `0111` |
| 6 | `0110` | `0101` |
| 7 | `0111` | `0100` |
| 8 | `1000` | `1100` |

可以逐行检查：每个相邻 Gray 码只有一个位置不同。

### 2.2 为什么叫反射

完整序列可以由较小的 Gray 序列反射构造：先列出 `n−1` 位序列（最高位 0），再
按逆序列出并加上最高位 1。这个构造与 `G = B xor (B >> 1)` 等价。它解释了
为什么边界处也只改变一位。

## 3. 算法

### 3.1 Binary → Gray

```text
gray = binary ^ (binary >> 1)
```

注意右移是逻辑移位。项目使用 `cpp_int`，不依赖带符号 64 位右移。

### 3.2 Gray → Binary

```text
binary_msb = gray_msb

for i from n-2 down to 0:
    binary[i] = binary[i+1] XOR gray[i]
```

也可以把每个 Gray 位与其左侧已经恢复的二进制位连续异或。

### 3.3 宽度与范围

Gray 输入/输出都固定为 `n` 位，数值范围是：

\[
0 \le V \le 2^n - 1
\]

负数、超出宽度的值、以及没有明确位宽的字符串都不是合法 Gray 编码请求。

### 3.4 相邻变化验证

对相邻数值 `v` 和 `v+1`：

```text
popcount(G(v) XOR G(v+1)) == 1
```

在完整序列的末端使用循环 Gray 时，最大码与最小码之间也可以通过特定反射构造
保持一位变化；本项目 API 只处理线性相邻值 `v` 与 `v+1`。

## 4. C++ 实现位置

| 内容 | 文件 |
| --- | --- |
| 接口与结果结构 | `cpp-core/include/radixlab/encoding/gray_code.hpp` |
| 双向转换 | `cpp-core/src/encoding/gray_code.cpp` |
| 测试 | `cpp-core/tests/test_gray_code.cpp` |

接口：

```cpp
BitVector binaryToGray(const BitVector& binary);
BitVector grayToBinary(const BitVector& gray);
GrayResult encodeGray(BigInt value, std::size_t width);
GrayResult decodeGray(const BitVector& gray);
```

结果同时包含 `value`、`binary`、`gray`、`width`，避免调用者只拿到一个看起来像
普通二进制的字符串。

## 5. API 与 CLI 实验

### 5.1 编码 13（8 位）

```bash
./cpp-core/build/radixlab-cli encoding \
  --operation gray-encode --value 13 --width 8 --json
```

关键结果：

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

### 5.2 反向验证

```bash
./cpp-core/build/radixlab-cli encoding \
  --operation gray-decode --bits 00001011 --json
```

`value` 应为 13。

### 5.3 观察相邻切换

13 的前一个值是 12：

```bash
./cpp-core/build/radixlab-cli encoding \
  --operation gray-encode --value 12 --width 8 --json
./cpp-core/build/radixlab-cli encoding \
  --operation gray-encode --value 13 --width 8 --json
```

两个 Gray 码分别是：

```text
12: 00001010
13: 00001011
        ^
```

只有 bit 0 改变。

### 5.4 REST API

```bash
curl -sS http://localhost:8080/api/v1/encoding/gray \
  -H 'Content-Type: application/json' \
  -d '{"value":"13","width":8}'
```

## 6. 网页实验：Gray Explorer

1. 打开 <http://localhost:8080/encoding.html#gray>。
2. 输入 13、8 位，观察 Binary 与 Gray 两个位格行。
3. 下方 “Adjacent transition” 显示当前值与前一个值的 Gray 码，并高亮变化位。
4. 点击 “Next →” 连续切换，观察每次只有一个格子高亮。
5. 把位宽改成 4，输入 15，再点 Next，按钮应禁用，因为 4 位最大值是 15。
6. 输入负数或超过 15 的值，观察范围错误。

## 7. 常见误区

- **把 Gray 码当作普通二进制数直接做算术**：Gray 的位置顺序不是位置权顺序。
- **认为 Gray 码是十六进制或 BCD**：它们是完全不同的编码。
- **混淆二进制数值与 Gray 码字**：`00001011` 作为 Gray 表示 13，作为普通二进制
  表示 11。
- **忽略位宽**：同一个数值在不同宽度下 Gray 码不同。
- **用有符号右移实现公式**：可能引入符号扩展，导致负值或高位错误。
- **以为相邻性质覆盖任意两个码字**：它只保证相邻序列位置；不是任意码字之间
  只差一位。

## 8. 练习

1. 求数值 10 的 4 位 Gray 码。
2. 4 位 Gray 码 `1111` 反解为哪个二进制值？
3. 写出 4 位 Gray 序列中 0–4 的码字。
4. 至少需要多少位才能表示 10 个 Gray 位置？为什么 Gray 码仍需要固定宽度？
5. 为什么 Gray Code 适合位置传感或异步状态切换，却不适合直接做二进制加法？

### 答案与验证

1. `B = 1010`，`B >> 1 = 0101`，`G = 1111`。
2. 从高到低：`b3=1`，`b2=1⊕1=0`，`b1=0⊕1=1`，`b0=1⊕1=0`，得到 `1010₂ = 10`。
3. `0000, 0001, 0011, 0010, 0110`。
4. 至少 4 位，因为 `2^3=8 < 10 ≤ 16=2^4`。即使不使用全部 16 个码字，Gray 的
   单比特相邻性质仍需要明确的固定宽度和序列顺序。
5. 编码减少的是相邻状态切换时的同时变化位数，降低多位竞争与采样歧义；但
   Gray 码不是按位权组织的算术编码，做加法需要先转回二进制，或者设计专门的
   非标准算术电路。

验证：

```bash
./cpp-core/build/radixlab-cli encoding \
  --operation gray-decode --bits 1111 --json
```

下一章加入冗余位：Parity 如何检测错误，以及它为什么不能纠正错误。
