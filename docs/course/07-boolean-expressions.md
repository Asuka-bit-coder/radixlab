# 07 Boolean Expressions, Lexer 与 AST

## 1. Theory

一个可执行的布尔表达式必须经过三个阶段：

```text
字符流 → Token 流 → AST → Boolean result
```

字符串替换或 `eval` 无法提供可靠的优先级、错误位置和结构化树。本模块使用正式的
词法分析器和递归下降解析器。

## 2. Grammar and Definition

```text
expression := or
or         := xor ( "|" xor )*
xor        := and ( "^" and )*
and        := unary ( "&" unary )*
unary      := "!" unary | primary
primary    := VARIABLE | "0" | "1" | "(" expression ")"
VARIABLE   := "A" | ... | "Z"
```

优先级：`!` > `&` > `^` > `|`。所有二元运算左结合。

Token 类型：

- Variable：`A`–`Z`
- Constant：`0`、`1`
- Not、And、Xor、Or
- LeftParen、RightParen
- End

空白被忽略，但每个 token 存储原始零基位置。最后附加 End token。

## 3. Algorithm

### 3.1 Lexical analysis

从左到右扫描字符：

1. 跳过 whitespace。
2. 识别单个字符 token。
3. 识别大写变量与常量。
4. 小写字母返回“变量必须大写”的 ParseError。
5. 未识别字符返回 ParseError 和准确位置。
6. 追加 End。

### 3.2 Recursive descent

每个语法层对应一个函数：

```text
parseOr → parseXor → parseAnd → parseUnary → parsePrimary
```

循环处理同层运算符，因此二元运算左结合。括号递归回到 `parseOr`。

### 3.3 AST ownership

```cpp
std::unique_ptr<Expr> operand;
std::unique_ptr<Expr> left;
std::unique_ptr<Expr> right;
```

选择理由：

- 节点由父节点唯一拥有，不会泄漏。
- 树是递归结构，`unique_ptr` 自然表达所有权。
- 禁止拷贝，避免意外共享可变树。
- 不需要垃圾回收，也不需要裸 owning pointer。

节点类型：`VariableExpr`、`ConstantExpr`、`NotExpr`、`BinaryExpr`。

## 4. C++ Implementation

| 内容 | 文件 |
| --- | --- |
| Lexer | `cpp-core/src/boolean/lexer.cpp` |
| Parser | `cpp-core/src/boolean/parser.cpp` |
| AST 求值/JSON | `cpp-core/src/boolean/ast.cpp` |
| 测试 | `cpp-core/tests/test_boolean_lexer.cpp`、`test_boolean_parser.cpp` |

限制：

- 用户请求表达式不超过 1024 字符（CLI/API）。
- parser 内部上限 4096 字符，以允许验证生成的 canonical 表达式。
- AST 节点上限 4096。
- 嵌套上限 64。

## 5. Java API

```http
POST /api/v1/boolean/evaluate
```

```json
{
  "expression": "A & (!B | C)",
  "assignment": {"A": true, "B": false, "C": false}
}
```

响应包含 `ast`：

```json
{
  "type": "and",
  "left": {"type": "variable", "name": "A"},
  "right": {
    "type": "or",
    "left": {"type": "not", "operand": {"type": "variable", "name": "B"}},
    "right": {"type": "variable", "name": "C"}
  }
}
```

## 6. Interactive Experiment

```bash
./cpp-core/build/radixlab-cli boolean evaluate \
  --expression "A & (B | !C)" \
  --assignment "A=1,B=0,C=0" --json
```

观察 `normalized` 为 `(A & (B | !C))`，AST 根为 AND。

输入非法表达式：

```bash
./cpp-core/build/radixlab-cli boolean truth-table \
  --expression "A & | B" --json
```

应得到 `parse_error`、position 4、column 5，退出码 2。Encoding/转换功能不受影响。

Boolean Lab 页面会在输入框下方显示同一列位置和 `^` 标记。

## 7. Common Mistakes

- 在运算符两侧省略空格不影响语法，`A&B` 合法。
- 不能用小写变量；`a` 是错误字符。
- 空表达式不是常量 0；它是语法错误。
- 括号不匹配时错误位置在当前位置，但消息会指出开括号的列。
- 不能用 `2` 或其它数字作为常量；只支持 `0` 和 `1`。
- AST 的显示括号很多，不代表语义错误；它是为了让优先级可见。

## 8. Exercises and Answers

1. 画出 `A & (B | !C)` 的 AST。  
   答案：AND 根；左子树 Variable A；右子树 OR；OR 左 Variable B；OR 右 NOT，NOT 子树 Variable C。
2. `A | B & C` 的根节点是什么？  
   答案：OR。AND 优先结合。
3. `((A))` 与 `A` 的 AST 语义是否相同？  
   答案：相同；括号只改变分组，空嵌套不产生额外运算。
4. 用 CLI 输入 `A ? B`，说明错误位置。  
   答案：`?` 位于索引 2，API 列号为 3。
5. 为什么 AST 不用裸 `Expr*` 拥有子节点？  
   答案：需要手动删除，异常和 early return 容易泄漏；`unique_ptr` 自动释放并表达唯一所有权。

验证：

```bash
./cpp-core/build/radixlab-cli boolean evaluate \
  --expression "A & (B | !C)" --assignment "A=1,B=0,C=0" --json
```
