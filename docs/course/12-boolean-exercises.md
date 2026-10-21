# Boolean Lab 综合练习

> 先手算，再用 CLI、API 或 Boolean Lab 验证。题目为本项目原创。

## 基础题

### 1. 优先级与括号

把 `A | B & C ^ !D` 写成完全括号形式。

**答案**

```text
A | ((B & C) ^ (!D))
```

AND 先于 XOR，XOR 先于 OR，NOT 最高。

**验证**

```bash
./cpp-core/build/radixlab-cli boolean evaluate \
  --expression "A | B & C ^ !D" \
  --assignment "A=0,B=1,C=1,D=1" --json
```

### 2. 语法错误位置

指出 `A & | B` 的第一个错误位置。

**答案**：`|` 位于零基索引 4，即第 5 列。API 返回
`"position":4,"column":5`。

**验证**

```bash
./cpp-core/build/radixlab-cli boolean truth-table \
  --expression "A & | B" --json
# exit 2
```

### 3. 行序

变量 A、B 的真值表第 2 行（index 2）是什么赋值？

**答案**：A=1，B=0。A 是第一个变量，也就是行索引的 MSB。

**验证**

```bash
./cpp-core/build/radixlab-cli boolean truth-table \
  --expression "A & B" --json
```

### 4. 常量表达式

`1` 的真值表有几行？变量数组是什么？

**答案**：1 行，变量数组为空，唯一一行的 assignment 也是空对象。

## 进阶题

### 5. Minterms

求 `A & B` 的最小项编号。

**答案**：只有 `{3}`，因为 A=1、B=1 是唯一使表达式为 1 的赋值。

**验证**

```bash
./cpp-core/build/radixlab-cli boolean canonical-forms \
  --expression "A & B" --json
```

### 6. Maxterms

求 `A & B` 的最大项编号。

**答案**：`{0,1,2}`，即结果为 0 的三行。

### 7. Canonical SOP

写出 `A ^ B` 的 canonical SOP。

**答案**

```text
(!A & B) | (A & !B)
```

### 8. Canonical POS

写出 `A ^ B` 的 canonical POS。

**答案**

```text
(A | B) & (!A | !B)
```

### 9. 德摩根律

验证 `!(A & B)` 与 `!A | !B`。

**答案**：等价。结果是 `equivalent=true`，没有 counterexample。

**验证**

```bash
./cpp-core/build/radixlab-cli boolean equivalent \
  --left "!(A & B)" --right "!A | !B" --json
```

### 10. 反例

比较 `A & B` 与 `A | B`，给出一个反例。

**答案**：A=0，B=1 时，AND 为 0，OR 为 1。引擎返回
`counterexample.assignment` 并分别给出 left/right 结果。

## 综合题

### 11. 吸收律

判断 `A | (A & B)` 与 `A`。

**答案**：等价。可以使用 CLI：

```bash
./cpp-core/build/radixlab-cli boolean equivalent \
  --left "A | (A & B)" --right "A" --json
```

### 12. 矛盾式与恒真式

比较 `A & !A` 与 `0`，以及 `A | !A` 与 `1`。

**答案**：两组都等价。canonical SOP/POS 的具体常量写法见
[Canonical SOP/POS](10-sop-pos.md)。

### 13. 变量上限

构造 7 个不同变量的表达式，预期 API 返回什么状态？

**答案**：`422 Unprocessable Entity`，错误码 `limit`。CLI 退出码为 3。

**验证**

```bash
./cpp-core/build/radixlab-cli boolean truth-table \
  --expression "A | B | C | D | E | F | G" --json
# exit 3
```

### 14. 变量名不同

`A` 与 `B` 是否等价？为什么不能用“表达式形状相同”直接判断？

**答案**：不等价。引擎使用变量并集 `{A,B}`；在 A=0,B=1 时两式不同。

### 15. 规范式与最简式

`A | !A` 的 canonical SOP 与最简单式分别是什么？

**答案**：在变量 A 上，canonical SOP 是 `(!A) | (A)`；最简单式是 `1`。
本项目的 canonical 结果不做最小化；二者等价但形式不同。

### 16. 综合三变量

对 `A & (B | !C)`：

1. 列出变量和行数；
2. 找出 minterm 编号；
3. 写出 canonical SOP 和 POS 的结构；
4. 在网页中点击切换 A/B/C，验证对应行。

**参考答案**

- 变量 A、B、C；8 行。
- 结果为 1 的行由真值表决定；用 CLI 或网页查看。
- SOP 由这些行的 minterm 组成，POS 由结果为 0 的 maxterm 组成。
- 网页选择行后，结果为 0 或 1，与真值表一致。

**验证**

```bash
./cpp-core/build/radixlab-cli boolean canonical-forms \
  --expression "A & (B | !C)" --json
```

## 自动验证清单

```bash
ctest --test-dir cpp-core/build --output-on-failure
cd java-backend
mvn test
RADIXLAB_CLI_PATH="$PWD/../cpp-core/build/radixlab-cli" mvn verify
```

通过测试不等于练习完成；还应能解释每一项的变量取向、行号和等价性。
