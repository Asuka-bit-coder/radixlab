# RadixLab 数字逻辑编码实验：总览

> 本讲义是 RadixLab v0.3.0 的配套学习资料，依据项目实际实现编写。
> 它不是对某一本教材的复制，也不虚构教材页码或未经核实的课程要求。

## 1. 为什么做这个实验

同一串二进制位可以有许多含义。`11110011` 可以是：

- 无符号整数 243；
- 8 位补码中的 −13；
- 8 位原码中的 −115；
- 某些 BCD 上下文中的非法码字；
- 一个带校验位的传输字的一部分。

位模式（bit pattern）本身不是数学数值。必须同时给出 **编码形式** 与
**位宽**，数值解释才成立：

```text
位模式 + 编码形式 + 位宽 → 数值或合法/非法判断
```

RadixLab 的目标是让你能够看到这条解释链，而不是只得到一个计算结果。

## 2. 学习闭环

每个知识点按以下顺序展开：

1. **Theory**：定义、取值范围、负零或非法码字等边界。
2. **Algorithm**：编码和解码的推导步骤。
3. **Implementation**：对应的 C++ 源文件与 Java API。
4. **Experiment**：CLI、REST API 或 Encoding Lab 中的可复现实验。
5. **Verification**：如何用另一个表示、边界值或自动化测试验证结论。

这种闭环避免“公式会背，但位模式写不出来”或“代码能跑，但边界不清楚”。

## 3. 实验环境

### 3.1 构建 C++ 引擎

```bash
cmake -S cpp-core -B cpp-core/build -DCMAKE_BUILD_TYPE=Debug
cmake --build cpp-core/build --parallel
ctest --test-dir cpp-core/build --output-on-failure
```

### 3.2 启动 Java 后端

```bash
cd java-backend
RADIXLAB_ENGINE_CLI_PATH=../cpp-core/build/radixlab-cli \
  mvn spring-boot:run
```

- 进制转换页面：<http://localhost:8080/>
- Encoding Lab：<http://localhost:8080/encoding.html>
- API 基址：`http://localhost:8080`
- CLI：`./cpp-core/build/radixlab-cli`

### 3.3 约定

| 约定 | 含义 |
| --- | --- |
| 位宽 `n` | 一个二进制字包含的位数 |
| bit index 0 | 最低有效位（Least Significant Bit, LSB） |
| 输出字符串 | 最左位为最高有效位（Most Significant Bit, MSB） |
| 原码 | Sign-Magnitude |
| 反码 | Ones' Complement |
| 补码 | Two's Complement |
| `negativeZero` | 某些编码中与普通零不同、但数值仍为零的位模式 |

界面中格子下方的小数字是 **位索引**，不是位权。例如 8 位字最左侧格子的位索引
是 7，位权是 `2^7`。

## 4. 模块地图

| 主题 | 理论重点 | C++ 实现 | Java API | 页面 |
| --- | --- | --- | --- | --- |
| 位模式与数制 | 位权、二进制/十六进制、固定位宽 | `encoding/bit_vector.*` | `/api/v1/convert` | Converter |
| 有符号数 | 原码、反码、补码、负零、上下界 | `encoding/signed_encoding.*` | `/api/v1/encoding/signed[/decode]` | Signed |
| BCD | 8421 权、合法四位组、前导零 | `encoding/bcd.*` | `/api/v1/encoding/bcd[/decode]` | BCD |
| Gray Code | 反射构造、单比特相邻变化 | `encoding/gray_code.*` | `/api/v1/encoding/gray[/decode]` | Gray |
| Parity | 偶/奇校验、错误检测上限 | `encoding/parity.*` | `/api/v1/encoding/parity` | Parity |
| Boolean algebra | 运算、优先级、布尔定律 | `boolean/ast.*`、`parser.*` | `/api/v1/boolean/evaluate` | Boolean |
| Boolean expressions | Lexer、递归下降、AST | `boolean/lexer.*`、`parser.*` | `/api/v1/boolean/evaluate` | Boolean |
| Truth tables | 变量顺序、行枚举 | `boolean/truth_table.*` | `/api/v1/boolean/truth-table` | Boolean |
| Minterms/Maxterms | 规范项编号 | `boolean/canonical_forms.*` | `/api/v1/boolean/canonical` | Boolean |
| SOP/POS | 规范表达式、常量边界 | `boolean/canonical_forms.*` | `/api/v1/boolean/canonical` | Boolean |
| Equivalence | 变量并集、反例 | `boolean/equivalence.*` | `/api/v1/boolean/equivalent` | Boolean |

## 5. 一次完整实验示例

以 −13、8 位补码为例。

### 5.1 理论预期

8 位补码的表示范围是 −128 到 127，因此 −13 可表示。

### 5.2 CLI 实验

```bash
./cpp-core/build/radixlab-cli encoding \
  --operation signed-encode \
  --value -13 --width 8 \
  --encoding TWOS_COMPLEMENT --json
```

输出中的关键字段：

```json
{
  "value": "-13",
  "width": 8,
  "encoding": "TWOS_COMPLEMENT",
  "bits": "11110011",
  "hex": "F3",
  "minimum": "-128",
  "maximum": "127"
}
```

### 5.3 REST API 实验

```bash
curl -sS http://localhost:8080/api/v1/encoding/signed \
  -H 'Content-Type: application/json' \
  -d '{"value":"-13","width":8,"encoding":"TWOS_COMPLEMENT"}'
```

### 5.4 反向验证

```bash
./cpp-core/build/radixlab-cli encoding \
  --operation signed-decode \
  --bits 11110011 --encoding TWOS_COMPLEMENT --json
```

应得到数值 `-13`。如果解码结果不是 −13，优先检查二进制字符串是否按 MSB→LSB
书写、位宽是否一致，而不是直接怀疑算术公式。

### 5.5 自动验证

```bash
ctest --test-dir cpp-core/build -R SignedEncodingTests --output-on-failure
```

## 6. 实验记录建议

每次实验建议记录：

1. 输入数值、编码形式、位宽；
2. 手算得到的位模式与范围判断；
3. 工具输出的位模式、十六进制和边界；
4. 反解码结果；
5. 如果失败，失败发生在输入校验、范围检查还是编码公式。

这比只截一张“成功”图更能说明你理解了表示过程。

## 7. 资料导航

- [数制与固定位模式](01-number-systems.md)
- [原码、反码与补码](02-signed-representation.md)
- [8421 BCD](03-bcd.md)
- [反射 Gray Code](04-gray-code.md)
- [奇偶校验](05-parity.md)
- [Boolean Algebra](06-boolean-algebra.md)
- [Boolean Expressions, Lexer 与 AST](07-boolean-expressions.md)
- [Truth Tables](08-truth-tables.md)
- [Minterms and Maxterms](09-minterms-maxterms.md)
- [Canonical SOP and POS](10-sop-pos.md)
- [Logical Equivalence](11-equivalence.md)
- [Boolean 综合练习](12-boolean-exercises.md)
- [综合练习与验证答案](exercises.md)

## 8. 实验边界

- 编码宽度支持 1–64 位；网页提供 4/8/16/32/64 位快捷选择。
- BCD 协议在本版本中只接受无符号十进制数字串。
- Parity 只实现单比特偶/奇校验，不实现 Hamming Code、纠错或错误定位。
- 所有编码都在 C++ 整数域中完成，不使用浮点数。
- API 输入长度、BCD 位数、并发与超时都有限制；这些限制是服务策略，不是数学结论。
- Boolean 表达式请求不超过 1024 字符；真值表与等价检查最多 6 个不同变量。
- Boolean AST 最多 4096 个节点，嵌套最多 64 层；这些限制用于保护服务端资源。

下一章从“位模式为什么不同于数值”开始，建立后续所有编码的共同语言。
