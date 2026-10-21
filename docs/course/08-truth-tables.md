# 08 Truth Tables 真值表

## 1. Theory

真值表枚举一个布尔函数在全部输入赋值上的输出。若表达式包含 `n` 个不同变量，就有：

\[
2^n
\]

种赋值。常量表达式没有变量，按一组空赋值处理，因此只有 1 行。

## 2. Variable Order and Row Numbering

RadixLab 的定义：

1. 变量按字母顺序排列：`A < B < C < ...`。
2. 第一个变量是行索引的 MSB。
3. 二进制行序固定，从全 0 到全 1。

对于变量 A、B：

| 行 index | A | B |
| --- | --- | --- |
| 0 | 0 | 0 |
| 1 | 0 | 1 |
| 2 | 1 | 0 |
| 3 | 1 | 1 |

行索引的数学定义：

\[
i = \sum_{k=0}^{n-1} v_k 2^{n-1-k}
\]

其中 `v[0]` 是 A 的取值，`v[1]` 是 B 的取值。

## 3. Algorithm

```text
variables = sorted(set(expression variables))
if |variables| > 6: reject
rows = 2^|variables|, or 1 for no variables

for index in 0 .. rows-1:
    for each variable position p:
        value = bit(index, n-1-p)
        assignment[variable[p]] = value
    result = evaluate(expression, assignment)
    append (index, assignment, result)
```

固定变量顺序和行顺序，使同一个表达式在不同时间、不同接口得到确定结果。

## 4. Examples

`A & B`：

| A | B | Result |
| --- | --- | --- |
| 0 | 0 | 0 |
| 0 | 1 | 0 |
| 1 | 0 | 0 |
| 1 | 1 | 1 |

`A ^ B`：

| A | B | Result |
| --- | --- | --- |
| 0 | 0 | 0 |
| 0 | 1 | 1 |
| 1 | 0 | 1 |
| 1 | 1 | 0 |

常量 `1`：只有一行，assignment 为空，result 为 1。

## 5. C++ Implementation

- 头文件：`cpp-core/include/radixlab/boolean/truth_table.hpp`
- 源文件：`cpp-core/src/boolean/truth_table.cpp`
- 测试：`cpp-core/tests/test_boolean_truth_table.cpp`

```cpp
TruthTable table = generateTruthTable(expression);
for (const auto& row : table.rows) {
    // row.index, row.assignment, row.result
}
```

变量上限 6，因此最大 64 行。这是应用层资源策略，不是布尔代数的数学限制。

## 6. Java API and CLI

```bash
./cpp-core/build/radixlab-cli boolean truth-table \
  --expression "A & B" --json
```

返回：

```json
{
  "variables": ["A", "B"],
  "rowCount": 4,
  "rows": [
    {"index":0,"assignment":{"A":false,"B":false},"result":false},
    {"index":1,"assignment":{"A":false,"B":true},"result":false},
    {"index":2,"assignment":{"A":true,"B":false},"result":false},
    {"index":3,"assignment":{"A":true,"B":true},"result":true}
  ],
  "ast": {"type":"and", "...": "..."}
}
```

API：

```bash
curl -sS http://localhost:8080/api/v1/boolean/truth-table \
  -H 'Content-Type: application/json' \
  -d '{"expression":"A & B"}'
```

## 7. Interactive Experiment

1. 打开 <http://localhost:8080/boolean.html>。
2. 输入 `A & B`，观察 2 个变量、4 行。
3. 点击 A/B 的切换按钮，或点击表格行，观察选中赋值和结果。
4. 输入 `A & (B | !C)`，观察 3 个变量、8 行，以及第一变量 A 在每一半行中的变化。
5. 输入常量 `0` 或 `1`，观察只有一行的空赋值。

## 8. Common Mistakes

- 把 A 当 LSB；本项目中 A 是第一个变量，也是行索引的 MSB。
- 只枚举使表达式为 1 的行；完整真值表必须包含所有赋值。
- 变量顺序随字母出现顺序变化；必须按 A–Z 排序。
- 把 6 个变量限制误解为只能使用 `A`–`F`；实际可以使用任意 6 个不同大写字母，但排序后最多 6 个。
- 忽略常量表达式的单行空赋值。

## 9. Exercises and Answers

1. 3 个变量有多少行？  
   答案：`2^3 = 8`。
2. 行 index 6 在变量 A,B,C 下对应什么赋值？  
   答案：`110`，即 A=1, B=1, C=0。
3. `A | B` 的真值表中哪些行结果为 1？  
   答案：索引 1、2、3。
4. 为什么变量限制可以防止资源问题？  
   答案：行数指数增长；6 个变量最多 64 行，7 个变量会翻倍。限制同时约束计算与响应大小。
5. 常量 0 的 `variables` 和 `rowCount` 是什么？  
   答案：空数组和 1。

验证：

```bash
./cpp-core/build/radixlab-cli boolean truth-table \
  --expression "A | B" --json
```
