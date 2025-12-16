#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QString>
#include <QStringList>
#include <QVector>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    enum class TokType {
        Number,
        Op,
        UnaryPostOp,
        LParen,
        RParen
    };
    struct Token {
        TokType type;
        QString text;
    };

private slots:
    void onAnyButtonClicked();
    void onExprReturnPressed();
    void onExprTextEdited(const QString &text);

private:
    void setupConnections();

    void insertToExpr(const QString &s);
    void backspaceExpr();
    void clearAll();
    void computeAndShow();

    QString normalizeExpression(const QString &in) const;

    bool tokenize(const QString &expr, QVector<Token> &outTokens, QString &err) const;
    bool toRpn(const QVector<Token> &tokens, QVector<Token> &outRpn, QString &err) const;
    bool evalRpn(const QVector<Token> &rpn, long double &outValue, QString &err) const;

    // Hex2LongDouble & LongDouble2Hex
    bool parseHexFloat(const QString &s, long double &out, QString &err) const;
    QString toHexFloatString(long double v, int fracDigits = 12) const;

    int precedence(const QString &op) const;
    bool isLeftAssociative(const QString &op) const;

private:
    Ui::MainWindow *ui;
    bool m_updatingText = false;
};
#endif // MAINWINDOW_H
