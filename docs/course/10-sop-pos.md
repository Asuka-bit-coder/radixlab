# 10 Canonical SOP and POS

## 1. Theory

规范表达式（canonical forms）由完整真值表构造：

- **Canonical SOP**（Sum of Products）：把结果为 1 的 minterm 全部 OR 起来。
- **Canonical POS**（Product of Sums）：把结果为 0 的 maxterm 全部 AND 起来。

它们描述同一个布尔函数，但结构不同。

## 2. Mathematical Definition

若 `f` 的最小项集合为 `m_1, m_2, ...`：

\[
f = m_{i_1} \lor m_{i_2} \lor \cdots
\]

若 `f` 的最大项集合为 `M_{j_1}, M_{j_2}, ...`：

\[
f = M_{j_1} \land M_{j_2} \land \cdots
\]

其中每个 minterm 是全部变量的 AND，每个 maxterm 是全部变量的 OR。

## 3. Example: XOR

`A XOR B`：

```text
m_1 = !A & B
m_2 = A & !B
M_0 = A | B
M_3 = !A | !B
```

Canonical SOP：

```text
(!A & B) | (A & !B)
```

Canonical POS：

```text
(A | B) & (!A | !B)
```

两者对所有 A、B 都产生相同的真值表。

## 4. Algorithm

```text
table = truthTable(f)
minterms = rows where result == 1
maxterms = rows where result == 0

SOP = join(" | ", term(m) for m in minterms)   // or "0" if empty
POS = join(" & ", clause(M) for M in maxterms) // or "1" if empty
```

对每个 minterm，变量值为 1 时写原变量，值为 0 时写反变量。  
对每个 maxterm，变量值为 0 时写原变量，值为 1 时写反变量。

本项目对每个 term/clause 加括号，以明确结构。

## 5. Constant and All-Rows Cases

规范形式和最简形式不是一回事。

- 若没有任何 minterm（函数恒 0），空和写作 `0`。
- 若没有任何 maxterm（函数恒 1），空积写作 `1`。
- 若函数恒 1，SOP 仍会展开所有 minterm；POS 因没有 maxterm 而写作 `1`。
- 若函数恒 0，SOP 写作 `0`；POS 会展开所有 maxterm。
- 无变量常量：SOP 和 POS 都直接写作 `1` 或 `0`，并分别记录 index 0 的
  minterm/maxterm。

RadixLab 会重新解析生成的规范表达式，并在原真值表的每一行上求值，确保结果等价。

## 6. Why This Is Not Minimization

例如 `A | !A` 的 canonical SOP 是：

```text
(!A) | (A)
```

它可以化简为 `1`，但“所有 minterm 的 OR”是规范展开；最小化会破坏“每项含全部变量”
的定义。v0.3.0 不实现 Karnaugh map 或 Quine-McCluskey。

## 7. C++ Implementation

```cpp
CanonicalForms forms = generateCanonicalForms(expression);
// forms.minterms, forms.maxterms, forms.sop, forms.pos, forms.table
```

- `cpp-core/include/radixlab/boolean/canonical_forms.hpp`
- `cpp-core/src/boolean/canonical_forms.cpp`

验证逻辑使用独立的 Parser 重新解析 `sop`/`pos`，不是仅比较字符串。

## 8. API and CLI

```bash
./cpp-core/build/radixlab-cli boolean canonical-forms \
  --expression "A ^ B" --json
```

API：

```http
POST /api/v1/boolean/canonical
{"expression":"A ^ B"}
```

响应同时提供：

```json
{
  "variables": ["A", "B"],
  "minterms": [1, 2],
  "maxterms": [0, 3],
  "sop": "(!A & B) | (A & !B)",
  "pos": "(A | B) & (!A | !B)"
}
```

## 9. Interactive Experiment

1. 在 Boolean Lab 输入 `A ^ B`。
2. 对照真值表中的 1/0 行与 minterm/maxterm 列表。
3. 将 SOP 复制到输入框，观察它生成完全相同的真值表。
4. 将 POS 复制到输入框，观察等价性。
5. 输入 `A | !A`，观察 canonical SOP 展开与 POS 的空积 `1`。

## 10. Common Mistakes

- 把 canonical SOP 当作最简 SOP。
- 漏掉某个 minterm 或把 maxterm 行号取反。
- 每个 minterm 应包含全部变量一次；漏变量会失去规范性。
- 把 SOP 的项写成 OR，把 POS 的因子写成 AND。
- 生成规范式后不再验证；本项目在核心中重新解析并逐行验证。

## 11. Exercises and Answers

1. `A & B` 的 canonical SOP 是什么？  
   答案：`(A & B)`，因为 minterm `{3}`。
2. `A & B` 的 canonical POS 是什么？  
   答案：`(A | B) & (A | !B) & (!A | B)`。
3. `0` 的 SOP 和 POS 是什么？  
   答案：都是 `0`。
4. `1` 的 SOP/POS 是什么？  
   答案：都是 `1`。
5. SOP 中 `A=0, B=1` 的 minterm 如何写？  
   答案：`(!A & B)`。

验证：

```bash
./cpp-core/build/radixlab-cli boolean canonical-forms \
  --expression "A & B" --json
```

```bash
./cpp-core/build/radixlab-cli boolean equivalent \
  --left "A & B" \
  --right "(A & B)" --json
```
