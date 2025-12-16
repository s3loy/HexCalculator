#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QPushButton>
#include <QRegularExpression>
#include <QStack>
#include <QtMath>

#include <cmath>
#include <limits>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    //init
    setupConnections();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setupConnections(){
    connect(ui->exprLineEdit,&QLineEdit::returnPressed,
            this,&MainWindow::onExprReturnPressed);
    connect(ui->exprLineEdit,&QLineEdit::textEdited,
            this,&MainWindow::onExprTextEdited);
    const auto buttons = ui->buttonWidget->findChildren<QPushButton*>();
    for (QPushButton *b : buttons){
        connect(b,&QPushButton::clicked,this,&MainWindow::onAnyButtonClicked);
    }
}

void MainWindow::onExprReturnPressed(){
    computeAndShow();
}

void MainWindow::onExprTextEdited(const QString &text){
    if (m_updatingText) return;

    if (text.contains('=')) {
        QString exprWithoutEqual = text;
        exprWithoutEqual. remove('=');

        m_updatingText = true;
        ui->exprLineEdit->setText(normalizeExpression(exprWithoutEqual));
        ui->exprLineEdit->setCursorPosition(ui->exprLineEdit->text().length());
        m_updatingText = false;

        computeAndShow();
        return;
    }

    const QString normalized = normalizeExpression(text);
    if (normalized == text) return;

    m_updatingText = true;

    int oldCursor = ui->exprLineEdit->cursorPosition();

    int charCountBeforeCursor = 0;
    for (int i = 0; i < oldCursor && i < text.length(); i++) {
        if (! text[i].isSpace()) {
            charCountBeforeCursor++;
        }
    }

    int newCursor = 0;
    int count = 0;
    for (int i = 0; i < normalized.length(); i++) {
        if (!normalized[i].isSpace()) {
            count++;
        }
        if (count >= charCountBeforeCursor) {
            newCursor = i + 1;
            break;
        }
    }

    if (count < charCountBeforeCursor) {
        newCursor = normalized.length();
    }

    ui->exprLineEdit->setText(normalized);
    ui->exprLineEdit->setCursorPosition(newCursor);

    m_updatingText = false;
}

void MainWindow::onAnyButtonClicked(){
    auto *b = qobject_cast<QPushButton*>(sender());
    if (!b) return;

    const QString name = b->objectName();

    if (name == "btnClear"){
        clearAll();
        return;
    }
    if (name == "btnDelete"){
        backspaceExpr();
        return;
    }
    if (name == "btnEqual"){
        computeAndShow();
        return;
    }

    QString t = b->text();

    static const QSet<QString> ops {"+","-","*","/","%","^","(",")","!"};
    if (ops.contains(t)){
        insertToExpr(" "+t+" ");
    } else {
        insertToExpr(t);
    }
}

void MainWindow::insertToExpr(const QString &s){
    QString text = ui->exprLineEdit->text() + s;

    QString normalized = normalizeExpression(text);

    m_updatingText = true;
    ui->exprLineEdit->setText(normalized);
    ui->exprLineEdit->setCursorPosition(normalized.length());
    m_updatingText = false;
}

void MainWindow::backspaceExpr(){
    QString t = ui->exprLineEdit->text();
    if (t.isEmpty()) return;

    int pos = t.length() - 1;
    while (pos >= 0 && t[pos].isSpace()) {
        pos--;
    }

    if (pos >= 0) {
        t. remove(pos, 1);
    }

    QString normalized = normalizeExpression(t);

    m_updatingText = true;
    ui->exprLineEdit->setText(normalized);
    ui->exprLineEdit->setCursorPosition(normalized.length());
    m_updatingText = false;
}

void MainWindow::clearAll(){
    ui->exprLineEdit->clear();
    ui->resultLineEdit->clear();
}

QString MainWindow::normalizeExpression(const QString &in) const{
    QString s = in.toUpper();

    s.replace(QRegularExpression("\\s+")," ");
    s = s.trimmed();

    s.replace("x","*");
    s.replace("÷","/");
    s.replace("！", "!");
    s.replace("（", "(");
    s.replace("）", ")");

    static const QRegularExpression opRe("\\s*([+\\-*/()%^!])\\s*");
    s.replace(opRe, " \\1 ");
    s.replace(QRegularExpression("\\s+"), " ");
    s = s.trimmed();

    return s;
}

void MainWindow::computeAndShow(){
    const QString expr = normalizeExpression(ui->exprLineEdit->text());
    ui->exprLineEdit->setText(expr);

    QVector<Token> tokens;
    QString err;
    if (!tokenize(expr, tokens, err)) {
        ui->resultLineEdit->setText("ERR: " + err);
        return;
    }

    QVector<Token> rpn;
    if (!toRpn(tokens, rpn, err)) {
        ui->resultLineEdit->setText("ERR: " + err);
        return;
    }

    long double v = 0;
    if (!evalRpn(rpn, v, err)) {
        if (err.isEmpty()) err = "Unknown Error during evaluation";
        ui->resultLineEdit->setText("ERR: " + err);
        return;
    }
    if (std::isinf(v)) {
        ui->resultLineEdit->setText("ERR: Factorial Overflow");
        return;
    }
    ui->resultLineEdit->setText(toHexFloatString(v, 12));
}

int MainWindow::precedence(const QString &op) const{
    if (op == "!") return 4;
    if (op == "^") return 3;
    if (op == "*" || op == "/" || op == "%") return 2;
    if (op == "+" || op == "-") return 1;
    return 0;
}

bool MainWindow::isLeftAssociative(const QString &op) const{
    if (op == "^") return false;  // 幂运算是右结合
    return true; // + - * / 都是左结合
}

bool MainWindow::tokenize(const QString &expr, QVector<Token> &outTokens, QString &err) const{
    outTokens.clear();
    if (expr.isEmpty()) {
        err = "empty expression";
        return false;
    }

    int i = 0;
    auto isHex = [](QChar c){
        return c.isDigit() || (c >= 'A' && c <= 'F');
    };

    while (i < expr.size()) {
        const QChar c = expr[i];

        if (c.isSpace()) {
            i++;
            continue;
        }
        if (c == '(') {
            outTokens.push_back({TokType::LParen, "("});
            i++;
            continue;
        }
        if (c == ')') {
            outTokens.push_back({TokType::RParen, ")"});
            i++;
            continue;
        }
        if (c == '+' || c == '-' || c == '*' || c == '/' || c == '%' || c == '^') {
            outTokens.push_back({TokType::Op, QString(c)});
            i++;
            continue;
        }
        if (c == '!') {
            outTokens.push_back({TokType::UnaryPostOp, QString(c)});
            i++;
            continue;
        }
        if (isHex(c) || c == '.') {
            int start = i;
            bool seenDot = false;

            while (i < expr.size()) {
                QChar cc = expr[i];
                if (cc == '.') {
                    if (seenDot) break;
                    seenDot = true;
                    i++;
                    continue;
                }
                if (isHex(cc)) {
                    i++;
                    continue;
                }
                break;
            }

            const QString num = expr.mid(start, i - start);
            if (num == ".") {
                err = "invalid number '.'";
                return false;
            }

            outTokens.push_back({TokType::Number, num});
            continue;
        }

        err = QString("unexpected char '%1'").arg(c);
        return false;
    }

    return true;
}

bool MainWindow::toRpn(const QVector<Token> &tokens, QVector<Token> &outRpn, QString &err) const{
    outRpn.clear();
    QStack<Token> opStack;

    for (const auto &t : tokens) {
        if (t.type == TokType::Number) {
            outRpn.push_back(t);
            continue;
        }

        if (t.type == TokType::Op) {
            while (!opStack.isEmpty()) {
                const Token top = opStack.top();
                if (top.type != TokType::Op) break;

                const int p1 = precedence(t.text);
                const int p2 = precedence(top.text);

                if ((isLeftAssociative(t.text) && p1 <= p2) || (!isLeftAssociative(t.text) && p1 < p2)) {
                    outRpn.push_back(opStack.pop());
                } else {
                    break;
                }
            }
            opStack.push(t);
            continue;
        }
        if (t.type == TokType::UnaryPostOp) {
            outRpn.push_back(t);
            continue;
        }

        if (t.type == TokType::LParen) {
            opStack.push(t);
            continue;
        }

        if (t.type == TokType::RParen) {
            bool matched = false;
            while (!opStack.isEmpty()) {
                Token top = opStack.pop();
                if (top.type == TokType::LParen) {
                    matched = true;
                    break;
                }
                outRpn.push_back(top);
            }
            if (!matched) {
                err = "mismatched parentheses";
                return false;
            }
            continue;
        }
    }

    while (!opStack.isEmpty()) {
        const Token top = opStack.pop();
        if (top.type == TokType::LParen || top.type == TokType::RParen) {
            err = "mismatched parentheses";
            return false;
        }
        outRpn.push_back(top);
    }

    return true;
}
static long double fastPow(long double base, long long exp) {
    bool negExp = exp < 0;
    if (negExp) exp = -exp;

    long double result = 1.0L;
    while (exp > 0) {
        if (exp & 1) {          // 二进制最低位为1
            result *= base;
        }
        base *= base;           // base 自乘
        exp >>= 1;              // 右移一位
    }
    return negExp ? (1.0L / result) : result;
}

// 通用幂运算 - 整数指数用快速幂，非整数用标准库
static long double safePow(long double a, long double b) {
    long long intExp = static_cast<long long>(b);
    if (static_cast<long double>(intExp) == b && std::abs(intExp) < 64) {
        return fastPow(a, intExp);  // 整数快速幂
    }
    return std::powl(a, b);         // 非整数标准库
}

static long double factorial(long long n) {
    if (n < 0) return std::numeric_limits<long double>::quiet_NaN();
    if (n > 22) return std::numeric_limits<long double>::infinity();
    if (n <= 1) return 1.0L;
    long double result = 1.0L;
    for (long long i = 1; i <= n; ++i) {
        result *= i;
        if (std::isinf(result) || result <= 0) {
            return std::numeric_limits<long double>::infinity();
        }
    }
    return result;
}

bool MainWindow::evalRpn(const QVector<Token> &rpn, long double &outValue, QString &err) const{
    QStack<long double> st;

    for (const auto &t : rpn) {
        if (t.type == TokType::Number) {
            long double v = 0;
            if (!parseHexFloat(t.text, v, err)) return false;
            st.push(v);
            continue;
        }

        if (t.type == TokType::Op) {
            if (st.size() < 2) {
                err = "not enough operands";
                return false;
            }
            const long double b = st.pop();
            const long double a = st.pop();

            long double r = 0;
            if (t.text == "+") r = a + b;
            else if (t.text == "-") r = a - b;
            else if (t.text == "*") r = a * b;
            else if (t.text == "/") {
                if (b == 0) {
                    err = "division by zero";
                    return false;
                }
                r = a / b;
            } else if (t.text == "%"){
                if (b ==0){
                    err = "modulo by zero";
                    return false;
                }
                r = std::fmodl(a,b);
            } else if (t.text == "^"){
                if (a == 0 && b <0){
                    err = "zero to negative power";
                    return false;
                }
                if (a < 0){
                    long long intExp = static_cast<long long>(b);
                    if (static_cast<long double>(intExp) != b){
                        err = "negative base with non-integer exponent";
                        return false;
                    }
                }
                r = safePow(a , b);
            } else {
                err = "unknown operator " + t.text;
                return false;
            }

            st.push(r);
            continue;
        }
        if (t.type == TokType::UnaryPostOp) {
            if (st.size() < 1) {
                err = "not enough operands for factorial";
                return false;
            }
            const long double a = st.pop();

            if (t. text == "!") {
                if (a < 0) {
                    err = "factorial of negative number";
                    return false;
                }
                long long intVal = static_cast<long long>(a);
                if (static_cast<long double>(intVal) != a) {
                    err = "factorial requires integer";
                    return false;
                }
                if (intVal > 22) {
                    err = "factorial overflow";
                    return false;
                }
                st.push(factorial(intVal));
            }
            continue;
        }

        err = "invalid token in rpn";
        return false;
    }

    if (st.size() != 1) {
        err = "invalid expression";
        return false;
    }

    outValue = st.pop();
    return true;
}

bool MainWindow::parseHexFloat(const QString &s, long double &out, QString &err) const{
    // 这里只做近似 转为 long double
    QString t = s.toUpper();

    bool neg = false;
    if (t.startsWith('+')) t.remove(0, 1);
    else if (t.startsWith('-')) { neg = true; t.remove(0, 1); }

    const QStringList parts = t.split('.', Qt::KeepEmptyParts);
    if (parts.size() > 2) { err = "invalid hex float"; return false; }

    auto hexDigit = [](QChar c) -> int {
        if (c.isDigit()) return c.unicode() - '0';
        if (c >= 'A' && c <= 'F') return 10 + (c.unicode() - 'A');
        return -1;
    };

    long double intPart = 0;
    if (!parts[0].isEmpty()) {
        for (QChar c : parts[0]) {
            int d = hexDigit(c);
            if (d < 0) {
                err = "invalid digit in integer part";
                return false;
            }
            intPart = intPart * 16 + d;
        }
    }

    long double fracPart = 0;
    if (parts.size() == 2 && !parts[1].isEmpty()) {
        long double base = 16;
        for (QChar c : parts[1]) {
            int d = hexDigit(c);
            if (d < 0){
                err = "invalid digit in fractional part";
                return false;
            }
            fracPart += (static_cast<long double>(d) / base);
            base *= 16;
        }
    }

    out = intPart + fracPart;
    if (neg) out = -out;
    return true;
}

QString MainWindow::toHexFloatString(long double v, int fracDigits) const{
    if (std::isnan(v)) return "NAN";
    if (std::isinf(v)) return (v > 0 ? "INF" : "-INF");

    bool neg = v < 0;
    if (neg) v = -v;

    if (v > static_cast<long double>(std::numeric_limits<quint64>::max())) {
        QString s = QString::number(static_cast<double>(v), 'g', 15);
        if (neg) s.prepend('-');
        return s;
    }

    quint64 intPart = static_cast<quint64>(v);
    long double frac = v - static_cast<long double>(intPart);

    QString intStr = QString::number(intPart, 16).toUpper();
    if (intStr.isEmpty()) intStr = "0";

    QString fracStr;
    for (int i = 0; i < fracDigits; i++) {
        frac *= 16;
        int digit = static_cast<int>(frac);

        if (digit < 0) digit = 0;
        if (digit > 15) digit = 15;

        frac -= digit;
        fracStr += QString("0123456789ABCDEF")[digit];
        if (frac == 0) break;
    }

    while (!fracStr.isEmpty() && fracStr.endsWith('0')) fracStr.chop(1);

    QString out = intStr;
    if (!fracStr.isEmpty()) out += "." + fracStr;
    if (neg) out.prepend('-');
    return out;
}
