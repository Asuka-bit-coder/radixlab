# 09 Minterms and Maxterms

## 1. Theory

一个 `n` 变量的真值表有 `2^n` 行。每一行对应：

- 一个 **minterm**（最小项）`m_i`：只在该行取值为 1；
- 一个 **maxterm**（最大项）`M_i`：只在该行取值为 0。

`i` 是该行变量赋值按 A 为 MSB 组成的二进制数。

## 2. Definitions

设变量为 `x_0, x_1, ..., x_{n-1}`，第 `i` 行中变量取值为 `v_j`。

### Minterm

\[
m_i = \bigwedge_{j=0}^{n-1} l_j,\quad
l_j = \begin{cases}
x_j, & v_j=1 \\
\bar{x_j}, & v_j=0
\end{cases}
\]

例如 A、B 的 minterm 编号：

| i | A B | m_i |
| --- | --- | --- |
| 0 | 0 0 | `!A & !B` |
| 1 | 0 1 | `!A & B` |
| 2 | 1 0 | `A & !B` |
| 3 | 1 1 | `A & B` |

### Maxterm

\[
M_i = \bigvee_{j=0}^{n-1} l_j,\quad
l_j = \begin{cases}
x_j, & v_j=0 \\
\bar{x_j}, & v_j=1
\end{cases}
\]

| i | A B | M_i |
| --- | --- | --- |
| 0 | 0 0 | `A \| B` |
| 1 | 0 1 | `A \| !B` |
| 2 | 1 0 | `!A \| B` |
| 3 | 1 1 | `!A \| !B` |

注意 minterm 使用“值为 1 的原变量、值为 0 的反变量”，maxterm 的取向相反。

## 3. Minterm and Maxterm Lists

给定函数 `f`：

- 最小项列表 = 所有 `f(i)=1` 的行号；
- 最大项列表 = 所有 `f(i)=0` 的行号。

例如 `A XOR B` 的真值表：

| i | A | B | f |
| --- | --- | --- | --- |
| 0 | 0 | 0 | 0 |
| 1 | 0 | 1 | 1 |
| 2 | 1 | 0 | 1 |
| 3 | 1 | 1 | 0 |

因此：

```text
Σm = {1, 2}
ΠM = {0, 3}
```

## 4. Constants and Degenerate Cases

- `f = 1`：所有行都在最小项列表中；没有 maxterm。
- `f = 0`：所有行都在最大项列表中；没有 minterm。
- 无变量常量：只有 index 0 一行。`f=1` 时 minterm 列表为 `{0}`，`f=0` 时
  maxterm 列表为 `{0}`。规范表达式使用常量 `1` 或 `0` 表示。

## 5. Algorithm

```text
table = generateTruthTable(expression)
minterms = []
maxterms = []
for row in table:
    if row.result: minterms.append(row.index)
    else:          maxterms.append(row.index)
```

真值表行号和变量顺序必须同时固定，否则编号没有意义。

## 6. C++ Implementation

- `generateCanonicalForms` 调用 `generateTruthTable`。
- 结果结构同时保存 `table`、`minterms`、`maxterms`、`sop`、`pos`。
- 生成表达式会被重新解析，并在原真值表所有行上求值验证。

文件：

- `cpp-core/include/radixlab/boolean/canonical_forms.hpp`
- `cpp-core/src/boolean/canonical_forms.cpp`

## 7. API and CLI

```bash
./cpp-core/build/radixlab-cli boolean canonical-forms \
  --expression "A ^ B" --json
```

关键字段：

```json
{
  "variables": ["A", "B"],
  "minterms": [1, 2],
  "maxterms": [0, 3],
  "sop": "(!A & B) | (A & !B)",
  "pos": "(A | B) & (!A | !B)"
}
```

API：

```http
POST /api/v1/boolean/canonical
{"expression":"A ^ B"}
```

## 8. Interactive Experiment

在 <http://localhost:8080/boolean.html> 输入 `A ^ B`：

1. 查看真值表中的 1 出现在行 1、2。
2. 查看 `Σm = 1, 2`。
3. 查看 `ΠM = 0, 3`。
4. 对照 SOP/POS 的每一项，确认每个 minterm 使用全部变量一次。

再输入 `1` 和 `0`，观察空列表和常量规则。

## 9. Common Mistakes

- 把 maxterm 编号取成“结果为 1 的行”。maxterm 列表对应结果为 0 的行。
- 变量顺序不统一，导致编号与 SOP/POS 不一致。
- 最小项漏掉某个变量，变成非规范项。
- 把 `m_1` 写成 `A & B`；在 A 为 MSB 的顺序中，`m_1` 是 `!A & B`。
- 认为最大项和最小项只是取反；两者变量取向和运算都不同。

## 10. Exercises and Answers

1. A、B、C 中，行 index 5 的赋值是什么？  
   答案：`101`，A=1,B=0,C=1。
2. 该行对应的 minterm 是什么？  
   答案：`A & !B & C`。
3. 该行对应的 maxterm 是什么？  
   答案：`!A | B | !C`。
4. `A & B` 的 minterm 编号是什么？  
   答案：只包含 `{3}`。
5. `A & B` 的 maxterm 编号是什么？  
   答案：`{0,1,2}`。

验证：

```bash
./cpp-core/build/radixlab-cli boolean canonical-forms \
  --expression "A & B" --json
```
