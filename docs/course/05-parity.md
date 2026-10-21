# 05 奇偶校验 Parity

## 1. 学习目标

完成本章后，你应能：

- 计算 even parity 与 odd parity 校验位；
- 从接收数据和收到的校验位判断是否检测到错误；
- 说明单比特奇偶校验能检测什么、不能检测什么；
- 解释为什么校验不能定位或纠正错误；
- 用 CLI、API 和 Parity Explorer 做单比特翻转实验。

## 2. 理论：附加一个冗余位

设有 `n` 位数据 `D`，其中 1 的个数为：

\[
c = \sum_{i=0}^{n-1} D_i
\]

生成一个校验位 `P`，使数据加校验位的总 1 个数满足指定奇偶性。

### 2.1 Even Parity

要求总 1 个数为偶数：

\[
P = c \bmod 2
\]

- 数据有偶数个 1：`P=0`；
- 数据有奇数个 1：`P=1`。

### 2.2 Odd Parity

要求总 1 个数为奇数：

\[
P = 1 - (c \bmod 2)
\]

- 数据有偶数个 1：`P=1`；
- 数据有奇数个 1：`P=0`。

### 2.3 接收端验证

接收端收到数据 `D'` 和校验位 `P_rx`，重新计算：

\[
P_{calc} = \operatorname{parity}(D')
\]

若 `P_calc = P_rx`，通过；否则检测到错误。

这里的“通过”表示没有检测到奇数个比特错误，不表示数据一定正确。

## 3. 例子

数据：`1011001`，共 4 个 1，位宽 7。

| 模式 | 校验位 | 数据+校验位 |
| --- | --- | --- |
| Even | 0 | `10110010` |
| Odd | 1 | `10110011` |

本项目把校验位附加在数据串的右侧显示；数据位索引仍从右侧 LSB 开始。

### 3.1 翻转一个数据位

翻转 bit 3（0 为 LSB），原数据从：

```text
1011001
```

变为：

```text
1010001
```

1 的个数从 4 变为 3。

- 原来 even parity 发送的校验位是 0；
- 接收端重新计算：3 个 1 需要校验位 1；
- `1 ≠ 0`，检测到错误。

但接收端只知道“奇数个位发生变化”，不知道具体是 bit 3 还是其他位置。

### 3.2 翻转两个数据位

同时翻转 bit 0 和 bit 1：

```text
1011001  →  1011010
```

1 的个数仍为 4，重新计算的 even parity 仍是 0，与实际收到的 0 相同，检查通过。
这就是偶数个错误漏检的例子。

## 4. 能力边界

下表假设接收到的校验位没有被改变，只讨论数据位错误：

| 数据位错误个数 | 能否检测 | 能否定位 | 能否纠正 |
| --- | --- | --- | --- |
| 1 | 单比特 parity 可检测 | 不能 | 不能 |
| 2 | 漏检（任何偶数个错误） | 不能 | 不能 |
| 3 | 可检测 | 不能 | 不能 |
| 任意偶数 | 漏检 | 不能 | 不能 |

对单个校验位而言，任何偶数个比特翻转都会保持 1 的个数的奇偶性，因此不会被发现；
奇数个翻转一定改变奇偶性，会被发现。但奇偶校验没有足够的信息确定错误位置。
Hamming Code 通过多个校验位和 syndrome 提供定位能力；它属于后续内容，本版本未实现。

## 5. C++ 实现位置

| 内容 | 文件 |
| --- | --- |
| ParityMode、结果结构 | `cpp-core/include/radixlab/encoding/parity.hpp` |
| 生成与验证 | `cpp-core/src/encoding/parity.cpp` |
| 测试 | `cpp-core/tests/test_parity.cpp` |

核心接口：

```cpp
ParityResult generateParity(const BitVector& data, ParityMode mode);
bool verifyParity(
    const BitVector& receivedData,
    bool receivedParityBit,
    ParityMode mode);
```

`generateParity` 返回数据 1 的个数、校验位和附加后的字符串。字符串形式可以
达到 65 个字符（64 位数据 + 1 位校验），但数据本身仍由最多 64 位的 BitVector
表示；校验位单独保存，不强行塞进 64 位 BitVector。

## 6. API 与 CLI 实验

### 6.1 生成 even parity

```bash
./cpp-core/build/radixlab-cli encoding \
  --operation parity --bits 1011001 --parity EVEN --json
```

关键输出：

```json
{
  "bits": "1011001",
  "originalBits": "1011001",
  "width": 7,
  "parity": "EVEN",
  "onesCount": 4,
  "receivedOnesCount": 4,
  "parityBit": "0",
  "computedParityBit": "0",
  "dataWithParity": "10110010",
  "valid": true,
  "flipped": false,
  "flippedIndex": null
}
```

### 6.2 翻转 bit 3

```bash
./cpp-core/build/radixlab-cli encoding \
  --operation parity --bits 1011001 --parity EVEN --flip-bit 3 --json
```

关键变化：

```text
originalBits      1011001
bits              1010001
receivedOnesCount 3
parityBit         0
computedParityBit 1
valid             false
flippedIndex      3
```

### 6.3 REST API

```bash
curl -sS http://localhost:8080/api/v1/encoding/parity \
  -H 'Content-Type: application/json' \
  -d '{"bits":"1011001","parity":"EVEN","flipBit":3}'
```

`flipBit` 可省略；省略时验证原始数据与它自己生成的校验位。

## 7. 网页实验：Parity Explorer

1. 打开 <http://localhost:8080/encoding.html#parity>。
2. 默认数据 `1011001`、even parity，观察校验位 0 和通过状态。
3. 在 “Flip a data bit” 选择 bit 3，观察高亮位、接收数据变化和校验失败。
4. 切换为 odd parity，观察校验位翻转为 1，但检测错误的能力仍只有一位奇偶性。
5. 尝试把校验模式与接收端传输位混淆：页面中的 `parityBit` 是发送前生成的位，
   `computedParityBit` 是接收端重新计算的位，两者比较才得到 `valid`。

## 8. 常见误区

- **认为 parity 能纠正错误**：它只能产生一位奇偶信息，不能定位。
- **认为检查通过等于数据正确**：任何偶数个错误都不会改变单校验位的奇偶性。
- **把 even parity 理解为“数据 1 的个数必须为偶数”**：更准确的是数据加校验位
  的总数为偶数；单看数据时，校验位会把奇数补成偶数。
- **混淆数据宽度和带校验位宽度**：7 位数据加 1 个校验位是 8 位传输字，但数据
  部分仍是 7 位。
- **把校验位位置当成固定标准**：本实验约定附加在右侧；真实协议会明确规定位置。
- **只测试一位错误**：必须同时观察双位错误漏检的反例。

## 9. 练习

1. 对数据 `11010` 生成 even parity 和 odd parity，写出完整传输字。
2. 收到数据 `11010` 和校验位 1，在 even parity 下是否通过？
3. 两个数据位翻转一定不会被检测到吗？请给出反例或说明。
4. 为什么 64 位数据需要表示成 65 个字符，而不能继续放进一个 64 位 BitVector？
5. 如果系统要求定位单个错误，单比特 parity 还需要增加什么机制？

### 答案与验证

1. `11010` 有 3 个 1。Even 校验位 1，传输字 `110101`；Odd 校验位 0，
   传输字 `110100`。
2. Even 校验位应为 1，收到的也是 1，因此通过。
3. 一定会漏检。翻转两个位会使 1 的个数变化 0 或 ±2，奇偶性不变；两个 1 同时
   变 0、两个 0 同时变 1，或者一个 1→0 加一个 0→1，even parity 都无法发现。
4. BitVector 是 64 位数据容器，校验位是独立的冗余位；把校验位追加成第 65 位会
   超出数据位宽约定。项目用字符串表示传输字，并单独返回校验位。
5. 需要增加更多冗余位和明确的校验方程，例如 Hamming Code 的多个校验组与
   syndrome，才能定位单个错误；纠错还需要确定错误位并翻转它。

验证：

```bash
./cpp-core/build/radixlab-cli encoding \
  --operation parity --bits 11010 --parity EVEN --json
```

下一章的综合练习会把数制、编码、接口和自动化验证串起来。
