# 11 Logical Equivalence

## 1. Theory

两个布尔表达式逻辑等价，当且仅当在变量的所有可能赋值上结果都相同：

\[
f \equiv g \iff \forall \sigma,\ [f]_\sigma = [g]_\sigma
\]

“表达式相同”是语法层面的判断；“逻辑等价”是函数层面的判断。二者不能混为一谈。

## 2. Algorithm

1. 解析两个表达式。
2. 合并双方变量，去重并按 A–Z 排序。
3. 若变量数超过限制（当前 6），拒绝。
4. 枚举 `2^n` 个赋值。
5. 分别求值 `left` 和 `right`。
6. 首次发现不同结果，返回该赋值作为 counterexample。
7. 所有赋值都相同，返回 `equivalent=true`。

若两个表达式的变量集合不同，使用并集。例如 `A` 与 `B` 的并集是 `{A,B}`，
不能在各自的一变量空间里错误地判定相等。

## 3. Counterexample

`A & B` 与 `A | B` 的第一个反例是：

```text
A = 0, B = 1
left  = 0 & 1 = 0
right = 0 | 1 = 1
```

一个反例就足以证明不等价。返回反例比只返回 `false` 更有教学和调试价值。

## 4. Verifying Boolean Laws

RadixLab 用真实引擎验证：

| 定律 | 示例 |
| --- | --- |
| Identity | `A ≡ A & 1` |
| Domination | `A & 0 ≡ 0` |
| Idempotent | `A ≡ A & A` |
| Complement | `A & !A ≡ 0` |
| Involution | `A ≡ !!A` |
| Commutative | `A & B ≡ B & A` |
| Associative | `(A & B) & C ≡ A & (B & C)` |
| Distributive | `A & (B \| C) ≡ (A & B) \| (A & C)` |
| Absorption | `A \| (A & B) ≡ A` |
| De Morgan | `!(A & B) ≡ !A \| !B` |

这些不是把字符串当作答案，而是完整枚举并比较真值。

## 5. C++ Implementation

- `cpp-core/include/radixlab/boolean/equivalence.hpp`
- `cpp-core/src/boolean/equivalence.cpp`
- 测试：`cpp-core/tests/test_boolean_equivalence.cpp`

```cpp
EquivalenceResult result = checkEquivalent(leftExpr, rightExpr);
if (!result.equivalent) {
    // result.counterexample has assignment, leftResult, rightResult
}
```

变量上限与真值表相同，最多 6 个不同变量。

## 6. API and CLI

```bash
./cpp-core/build/radixlab-cli boolean equivalent \
  --left "!(A & B)" \
  --right "!A | !B" --json
```

```http
POST /api/v1/boolean/equivalent
```

```json
{
  "leftExpression": "A & B",
  "rightExpression": "A | B"
}
```

不等价响应中的 `counterexample`：

```json
{
  "assignment": {"A": false, "B": true},
  "left": false,
  "right": true
}
```

若等价，`counterexample` 为 `null`。

解析错误会标明是 Left 还是 Right expression，并给出列位置。

## 7. Interactive Experiment

1. 打开 <http://localhost:8080/boolean.html>。
2. 默认比较 `!(A & B)` 与 `!A | !B`，确认等价。
3. 将右侧改为 `!A & !B`，观察不再等价并出现反例。
4. 比较 `A & B` 与 `B & A`，确认变量顺序不影响等价。
5. 比较 `A` 与 `B`，观察引擎使用变量并集，不会把同名模式误判为等价。

## 8. Common Mistakes

- 用字符串相等代替逻辑等价。
- 只枚举使某个表达式为 1 的行，漏掉另一表达式为 1 的行。
- 变量集合不同却不取并集。
- 变量名不同但真值表碰巧一致时，错误地按“同一个变量”处理；应使用两个变量的并集。
- 认为找到等价就是找到了最简表达式；等价不是最小化。

## 9. Exercises and Answers

1. `A | (A & B)` 是否等价于 `A`？  
   答案：是，吸收律。
2. `A & (B | C)` 与 `(A & B) | C` 是否等价？  
   答案：否。引擎按行序返回的首个反例是 A=0、B=0、C=1（index 1）：左侧为 0，右侧为 1。
3. `!(A | B)` 与 `!A & !B` 是否等价？  
   答案：是，De Morgan。
4. 为什么 `A` 与 `B` 不等价？  
   答案：在 A=0,B=1 时结果不同。
5. 反例为何足以证明不等价？  
   答案：等价要求所有赋值相同；一个不同的赋值就构成反证。

验证：

```bash
./cpp-core/build/radixlab-cli boolean equivalent \
  --left "A" --right "B" --json
```

```bash
./cpp-core/build/radixlab-cli boolean equivalent \
  --left "!(A | B)" --right "!A & !B" --json
```
