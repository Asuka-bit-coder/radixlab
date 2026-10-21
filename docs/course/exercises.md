# 数字逻辑编码综合练习

> 建议先独立计算，再运行命令验证。所有命令假定仓库根目录为当前目录，且
> `cpp-core/build/radixlab-cli` 已构建。API 示例假定后端运行在 8080 端口。

## 实验一：固定位宽与补零

**任务**

1. 将十进制 `59` 写成 8 位无符号二进制。
2. 写出它的两位十六进制表示。
3. 解释 `111011`、`00111011` 与 `3B` 的关系。
4. 如果只保留低 4 位，结果是什么？为什么不能把结果当作原值的合法窄编码？

**手工答案**

- `59 = 32+16+8+2+1 = 111011₂`。
- 补零到 8 位：`00111011`。
- `0011 1011 = 3B₁₆`。
- 低 4 位是 `1011 = 11`。高位被丢弃后得到的是另一个位模式，不代表 59 的
  4 位编码。

**验证**

```bash
./cpp-core/build/radixlab-cli --input 59 --from 10 --to 2
./cpp-core/build/radixlab-cli --input 59 --from 10 --to 16
```

## 实验二：三种有符号编码

**任务**

对十进制 `-13`、位宽 8：

1. 手写原码、反码、补码；
2. 用 CLI 分别编码；
3. 用补码解码反向验证；
4. 写出三种编码的可表示范围。

**手工答案**

```text
原码: 10001101
反码: 11110010
补码: 11110011
```

范围：

- 原码/反码：−127 … 127；
- 补码：−128 … 127。

**验证**

```bash
./cpp-core/build/radixlab-cli encoding \
  --operation signed-encode --value -13 --width 8 \
  --encoding SIGN_MAGNITUDE --json

./cpp-core/build/radixlab-cli encoding \
  --operation signed-encode --value -13 --width 8 \
  --encoding ONES_COMPLEMENT --json

./cpp-core/build/radixlab-cli encoding \
  --operation signed-encode --value -13 --width 8 \
  --encoding TWOS_COMPLEMENT --json
```

最后一条应返回 `bits: "11110011"`、`hex: "F3"`。

## 实验三：最小值与范围错误

**任务**

1. 分别求 4 位原码、反码、补码的最小值；
2. 编码 `-8`，记录哪些形式成功、哪些失败；
3. 说明为什么补码能多表示一个负数；
4. 验证 API 的 HTTP 状态码。

**答案**

- 4 位原码/反码：−7 … 7；
- 4 位补码：−8 … 7；
- `-8`：原码/反码无法表示；补码为 `1000`。
- 原码/反码有两个零，其中一个位模式被“负零”占用；补码只有一个零，因此多出的
  模式分配给最小值 −8。

**验证**

```bash
./cpp-core/build/radixlab-cli encoding \
  --operation signed-encode --value -8 --width 4 \
  --encoding TWOS_COMPLEMENT --json
# bits: 1000

./cpp-core/build/radixlab-cli encoding \
  --operation signed-encode --value -8 --width 4 \
  --encoding SIGN_MAGNITUDE --json
# exit 4, range_error
```

API 对同样的原码请求返回 `422 Unprocessable Entity`。

## 实验四：负零

**任务**

1. 写出 8 位原码和反码的负零模式；
2. 用 CLI 解码两个模式，确认 `value` 和 `negativeZero`；
3. 请求补码负零，解释 HTTP 结果；
4. 说明负零与普通零的数学值关系。

**答案**

- 8 位原码负零：`10000000`；
- 8 位反码负零：`11111111`；
- 两者解码数值都是 0，但 `negativeZero=true`；
- 补码没有独立负零，请求应被拒绝（CLI exit 2 / API 400）。

**验证**

```bash
./cpp-core/build/radixlab-cli encoding \
  --operation signed-decode --bits 10000000 \
  --encoding SIGN_MAGNITUDE --json

./cpp-core/build/radixlab-cli encoding \
  --operation signed-decode --bits 11111111 \
  --encoding ONES_COMPLEMENT --json
```

## 实验五：BCD 与普通二进制

**任务**

1. 把 `206` 编码为 8421 BCD；
2. 写出 206 的普通二进制；
3. 比较两者的位数；
4. 判断 `1010` 是否为合法 BCD nibble；
5. 说明 `0206` 与 `206` 的 BCD 差异。

**手工答案**

- `206 → 0010 0000 0110`（12 位 BCD）。
- `206₁₀ = 11001110₂`（8 位）。
- BCD 以更多位换取十进制逐位处理。
- `1010 = 10`，不是合法十进制数字，因此非法。
- `0206` 有四个 nibble，位串包含 4 个组；`206` 有三个组。数学值都是 206，
  但 BCD 位模式与组数不同。

**验证**

```bash
./cpp-core/build/radixlab-cli encoding \
  --operation bcd-encode --decimal 206 --json

./cpp-core/build/radixlab-cli encoding \
  --operation bcd-decode --bits 1010 --json
# exit 2
```

## 实验六：Gray Code 相邻切换

**任务**

1. 求数值 12 与 13 的 8 位 Gray 码；
2. 用 XOR 验证两码只差一位；
3. 反解 13 的 Gray 码；
4. 说明 Gray 码 `1111`（4 位）作为普通二进制、和作为 Gray 反解值分别是多少。

**答案**

- 12：二进制 `00001100`，Gray `00001010`；
- 13：二进制 `00001101`，Gray `00001011`；
- 两者 XOR 为 `00000001`，仅 bit 0 改变；
- 13 的 Gray 反解回 13；
- `1111` 作为普通二进制是 15；作为 4 位 Gray 反解是 10。

**验证**

```bash
./cpp-core/build/radixlab-cli encoding \
  --operation gray-encode --value 12 --width 8 --json
./cpp-core/build/radixlab-cli encoding \
  --operation gray-encode --value 13 --width 8 --json
./cpp-core/build/radixlab-cli encoding \
  --operation gray-decode --bits 1111 --json
```

## 实验七：Parity 错误检测

**任务**

1. 对 `1011001` 生成 even parity 和 odd parity；
2. 翻转 bit 3，判断接收端结果；
3. 同时翻转 bit 0 与 bit 1，判断结果；
4. 解释为什么 parity 不能定位错误。

**答案**

- 数据有 4 个 1：even 校验位 0，odd 校验位 1；
- 翻转 bit 3 后数据为 `1010001`，3 个 1；even 校验位重算为 1，与发送的 0
  不符，检测到错误；
- 翻转 bit 0 与 bit 1 后数据为 `1011010`，仍有 4 个 1，even 校验位仍为 0，
  两个错误全部漏检；
- 一个校验位只提供总奇偶性，不能产生足够多的方程来定位某一位。

**验证**

```bash
./cpp-core/build/radixlab-cli encoding \
  --operation parity --bits 1011001 --parity EVEN --flip-bit 3 --json
# valid: false

./cpp-core/build/radixlab-cli encoding \
  --operation parity --bits 1011001 --parity ODD --flip-bit 3 --json
# valid: false
```

双位翻转需要另写输入（本项目实验接口一次只接受一个 `flip-bit`），例如接收数据
`1011010`，直接验证它自身的 even parity：

```bash
./cpp-core/build/radixlab-cli encoding \
  --operation parity --bits 1011010 --parity EVEN --json
# 数据自身生成校验位并验证，valid: true
```

## 实验八：REST API 与错误边界

**任务**

1. 用 curl 编码 `-13` 的 8 位补码；
2. 请求 `-128` 的 8 位原码，记录状态码；
3. 解码原码负零，检查 JSON 中的 `negativeZero`；
4. 提交非法 BCD nibble，记录状态码；
5. 验证 `width: 65` 在请求校验阶段被拒绝。

**验证脚本**

```bash
curl -sS http://localhost:8080/api/v1/encoding/signed \
  -H 'Content-Type: application/json' \
  -d '{"value":"-13","width":8,"encoding":"TWOS_COMPLEMENT"}'

curl -sS -o /tmp/range.json -w '%{http_code}\n' \
  http://localhost:8080/api/v1/encoding/signed \
  -H 'Content-Type: application/json' \
  -d '{"value":"-128","width":8,"encoding":"SIGN_MAGNITUDE"}'

curl -sS http://localhost:8080/api/v1/encoding/signed/decode \
  -H 'Content-Type: application/json' \
  -d '{"bits":"10000000","encoding":"SIGN_MAGNITUDE"}'

curl -sS -o /tmp/bcd.json -w '%{http_code}\n' \
  http://localhost:8080/api/v1/encoding/bcd/decode \
  -H 'Content-Type: application/json' \
  -d '{"bits":"1010"}'

curl -sS -o /tmp/width.json -w '%{http_code}\n' \
  http://localhost:8080/api/v1/encoding/signed \
  -H 'Content-Type: application/json' \
  -d '{"value":"0","width":65,"encoding":"TWOS_COMPLEMENT"}'
```

预期：第一条 200；第二条 422；第三条 200 且 `negativeZero=true`；第四条 400；
第五条 400（Bean Validation 在启动进程前拒绝宽度 65）。

## 实验九：网页交互核查

打开 <http://localhost:8080/encoding.html>，依次完成：

1. Signed：8 位补码 −13，确认 `11110011`、`0xF3`、范围 −128…127；
2. Signed：切到原码，输入 0，勾选负零，确认 `10000000` 与负零提示；
3. BCD：59，确认两个组 `0101`、`1001`，普通二进制 `111011`；
4. Gray：13，点击 Next，确认相邻切换只有一个高亮位；
5. Parity：选择 flip bit 3，确认错误状态和接收/计算校验位不同；
6. 用键盘 Tab 进入标签栏，再用 Left/Right/Home/End 键切换标签，确认焦点可见、结果区域会更新。

## 实验十：综合设计题

设计一个 12 位通信帧：

- 前 8 位是一个补码整数；
- 后 4 位是一个 BCD 数字；
- 另加 1 个 even parity 位覆盖前 12 位。

回答：

1. 该补码整数范围是什么？
2. BCD 部分能表示哪些值？
3. 如果接收端发现 parity 错误，它知道错误位置吗？
4. 哪些错误组合可能漏检？
5. 为什么不建议把 13 位的整个帧塞进一个 12 位 BitVector？

**参考答案**

1. 8 位补码范围：−128 … 127。
2. 4 位 BCD 只合法表示 0 … 9。
3. 不知道位置，只知道总奇偶性发生变化。
4. 任意偶数个位翻转都会保持单个 parity 位的值，因此必然漏检。
5. 12 位 BitVector 无法容纳额外校验位；校验位应作为独立字段，或者使用明确宽度
   为 13 的帧结构和对应校验协议。RadixLab 的 API 分开返回数据和校验位。

## 自动验证清单

```bash
# 全部 C++ 与 CLI 测试
ctest --test-dir cpp-core/build --output-on-failure

# Java 单元/控制器测试
cd java-backend
mvn test

# 真实 Java → C++ HTTP 集成测试
RADIXLAB_CLI_PATH="$PWD/../cpp-core/build/radixlab-cli" mvn verify
```

通过这些测试不等于实验报告自动完成；报告仍应包含手算、位模式、边界和失败分析。
