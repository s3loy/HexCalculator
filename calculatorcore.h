#ifndef CALCULATORCORE_H
#define CALCULATORCORE_H

#include <QString>
#include <QVector>
#include <QStringList>

class CalculatorCore
{
public:
    CalculatorCore();

    struct Result {
        QString valueStr;
        bool isError;
        QString errorMsg;
    };

    Result compute(const QString &expression);

    static QString toHexFloatString(long double v, int fracDigits = 12);

private:
    enum class TokType {
        Number,
        Op,
        UnaryPreOp,
        UnaryPostOp,
        LParen,
        RParen
    };
    struct Token {
        TokType type;
        QString text;
    };

    bool tokenize(const QString &expr, QVector<Token> &outTokens, QString &err) const;
    bool toRpn(const QVector<Token> &tokens, QVector<Token> &outRpn, QString &err) const;
    bool evalRpn(const QVector<Token> &rpn, long double &outValue, QString &err) const;

    int precedence(const QString &op) const;
    bool isLeftAssociative(const QString &op) const;
    static bool parseHexFloat(const QString &s, long double &out, QString &err);
    static long double fastPow(long double base, long long exp);
    static long double safePow(long double a, long double b);
    static long double factorial(long long n);
};

#endif // CALCULATORCORE_H
