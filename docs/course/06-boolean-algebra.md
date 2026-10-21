# 06 布尔代数 Boolean Algebra

## 1. Theory

布尔代数处理只取 `0` 与 `1` 的两个值。RadixLab 的 Boolean Lab 使用以下运算：

| 运算 | English | 符号 | 元数 |
| --- | --- | --- | --- |
| NOT | complement | `!` | unary |
| AND | conjunction | `&` | binary |
| XOR | exclusive OR | `^` | binary |
| OR | disjunction | `\|` | binary |

NOT 的优先级最高，其后依次是 AND、XOR、OR：

```text
! > & > ^ > |
```

二元运算左结合。例如：

```text
A | B & C = A | (B & C)
A ^ B & C = A ^ (B & C)
A | B | C = (A | B) | C
```

不支持省略运算符。`AB` 不是 `A & B`，而是语法错误。

## 2. Mathematical Definition

对赋值 `σ: {A,...,Z} → {0,1}`，递归定义：

\[
\begin{aligned}
[x]_\sigma &= \sigma(x) \\
[0]_\sigma &= 0,\quad [1]_\sigma = 1 \\
[!e]_\sigma &= 1 - [e]_\sigma \\
[e_1 \& e_2]_\sigma &= \min([e_1]_\sigma,[e_2]_\sigma) \\
[e_1 | e_2]_\sigma &= \max([e_1]_\sigma,[e_2]_\sigma) \\
[e_1 \oplus e_2]_\sigma &= [e_1]_\sigma \oplus [e_2]_\sigma
\end{aligned}
\]

最后一个 `⊕` 是模 2 加法：两个输入不同时为 1。

## 3. Standard Laws

| 定律 | AND 形式 | OR 形式 |
| --- | --- | --- |
| Identity | `A & 1 = A` | `A \| 0 = A` |
| Domination | `A & 0 = 0` | `A \| 1 = 1` |
| Idempotent | `A & A = A` | `A \| A = A` |
| Complement | `A & !A = 0` | `A \| !A = 1` |
| Involution | `!!A = A` | — |
| Commutative | `A & B = B & A` | `A \| B = B \| A` |
| Associative | `(A & B) & C = A & (B & C)` | `(A \| B) \| C = A \| (B \| C)` |
| Distributive | `A & (B \| C) = (A & B) \| (A & C)` | `A \| (B & C) = (A \| B) & (A \| C)` |
| Absorption | `A & (A \| B) = A` | `A \| (A & B) = A` |
| De Morgan | `!(A & B) = !A \| !B` | `!(A \| B) = !A & !B` |

这些定律不是测试里写死的字符串，而是通过构造完整真值表并比较所有赋值来验证。

## 4. Algorithm

Boolean Lab 的处理链：

1. Lexer 把字符转为 token，并记录每个 token 的位置。
2. Recursive-descent parser 根据优先级和括号构造 AST。
3. AST 递归求值，或提取变量集合并枚举真值表。
4. 等价性检查比较两个 AST 在变量并集上的所有赋值。

## 5. C++ Implementation

| 内容 | 文件 |
| --- | --- |
| Token | `cpp-core/include/radixlab/boolean/token.hpp` |
| Lexer | `cpp-core/include/radixlab/boolean/lexer.hpp` |
| AST | `cpp-core/include/radixlab/boolean/ast.hpp` |
| Parser | `cpp-core/include/radixlab/boolean/parser.hpp` |
| Evaluator | `cpp-core/include/radixlab/boolean/evaluator.hpp` |
| Limits | `cpp-core/include/radixlab/boolean/limits.hpp` |

AST 节点使用 `std::unique_ptr` 表达父子所有权，因为布尔表达式是天然的树，节点类型有限且需要递归遍历。

## 6. Java API

真值表接口同时返回 AST、变量和每一行结果：

```json
POST /api/v1/boolean/truth-table
{"expression":"A & (B | !C)"}
```

```bash
curl -sS http://localhost:8080/api/v1/boolean/truth-table \
  -H 'Content-Type: application/json' \
  -d '{"expression":"A & (B | !C)"}'
```

## 7. Interactive Experiment

```bash
./cpp-core/build/radixlab-cli boolean evaluate \
  --expression "A & (!B | C)" \
  --assignment "A=1,B=0,C=0" --json
```

结果应为 `true`。再改变 `A=0`，观察短路求值使结果变为 `false`。

在 <http://localhost:8080/boolean.html> 输入同一表达式，观察 3 个变量、8 行真值表以及 AST 分支。

## 8. Common Mistakes

- 把 `^` 当作指数运算；这里是 XOR。
- 认为 AND 和 OR 优先级相同；实际 AND 高于 XOR，高于 OR。
- 写 `AB` 表示 AND；本语法要求 `A & B`。
- 把 `+`、`*`、`~` 等其它教材符号直接输入；当前版本只接受 `! & ^ |`。
- 把“表达式看起来不同”直接判定为不等价，或反过来把变量名不同但模式相同的表达式误判为等价。

## 9. Exercises and Answers

1. 按本语法给 `A | B & !C` 加括号。  
   答案：`A | (B & !C)`。
2. `A ^ B ^ C` 是否左结合？写出括号形式。  
   答案：是，`((A ^ B) ^ C)`。由于 XOR 结合，数值上也等于 `A ^ (B ^ C)`，但 parser 的 AST 是左结合的。
3. 用 CLI 验证德摩根律。  
   ```bash
   ./cpp-core/build/radixlab-cli boolean equivalent \
     --left "!(A & B)" --right "!A | !B" --json
   ```
   应返回 `"equivalent":true`。
4. 说明 `AB` 为什么不是合法表达式。  
   答案：语法没有隐式 AND；解析 `A` 后遇到第二个变量 `B`，会报告“完整表达式后出现额外 token”。
