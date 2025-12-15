# HexCalculator

Programming Coursework

## Project Requirements

双目运算

表达式运算

选做：
1. 考虑括号
2. 考虑取余运算、求平方运算
3. 图形界面
4. 自拟扩展功能

## ToDo list
- [x] 十六进制输入 input
- [x] 小数点 .
- [x] 四则运算 + - * /
- [x] 计算 =
- [x] 清空 AC
- [x] 退格 DEL
- [x] 取模 %
- [x] 平方 ^2
- [x] 幂运算 ^n
- [ ] 阶乘 !
- [ ] 左移位 <<
- [ ] 右移位 >>
- [ ] 按位与 &
- [ ] 按位或 |
- [ ] 按位异或 ^
- [ ] 按位取反 ~
- [ ] 正负切换 +/-
- [ ] or anything else?

## Implementation Detail
### Data structure
```cpp
enum class TokType {
    Number,     // hexadecimal
    Op,         // operation
    LParen,     // (
    RParen      // )
};
struct Token {      // tokenization 
    TokType type;
    QString text;
};
```
### Algorithm
Tokenization → Convert to RPN → Evaluate

#### `bool MainWindow::tokenize(const QString &expr, QVector<Token> &outTokens, QString &err) const`

isHex lambda 判断是否为十六进制字符 (0-9, A-F)

逐字扫
- 空格 → 跳过
- ( 或 ) → 生成括号 Token
- +, -, *, / → 生成运算符 Token
- 十六进制字符或 . → 收集完整数字，支持小数点

#### `bool MainWindow::toRpn(const QVector<Token> &tokens, QVector<Token> &outRpn, QString &err) const`
度场算法 (Shunting Yard)
将中缀表达式转换为逆波兰表达式（后缀表达式）

Dijkstra 调度场算法：

- 数字 → 直接输出到结果队列
- 运算符 → 比较优先级，将栈顶高优先级/同优先级运算符弹出到结果，再入栈
- 左括号 → 入栈
- 右括号 → 弹出运算符直到遇到左括号
- 最后将栈中剩余运算符弹出

#### `bool MainWindow::evalRpn(const QVector<Token> &rpn, long double &outValue, QString &err) const`
`evalRpn()` - 逆波兰表达式求值

使用栈存储操作数
- 遇到数字 → 解析为 long double 入栈
- 遇到运算符 → 弹出两个操作数，计算结果入栈
- 最终栈中剩余一个值即为结果

#### 十六进制浮点数处理
##### `bool MainWindow::parseHexFloat(const QString &s, long double &out, QString &err) const`

处理正负号,
按 . 分割为整数部分和小数部分

整数部分：intPart = intPart * 16 + digit

小数部分：fracPart += digit / base，base 依次为 16, 256, 4096...
##### `QString MainWindow::toHexFloatString(long double v, int fracDigits = 12) const`

处理特殊值 (NaN, Inf)
分离整数和小数部分
整数部分：用 Qt 的 QString::number(intPart, 16) 直接转换
小数部分：循环乘 16 取整数位，最多 12 位
去除末尾的零

