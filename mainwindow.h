#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "calculatorcore.h"

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

private slots:
    void onAnyButtonClicked();
    void onExprReturnPressed();
    void onExprTextEdited(const QString &text);

private:
    void setupConnections();
    void computeAndShow();

    void insertToExpr(const QString &s);
    void backspaceExpr();
    void clearAll();
    QString normalizeExpression(const QString &in) const;

private:
    Ui::MainWindow *ui;
    CalculatorCore m_calc;
    bool m_updatingText = false;
};
#endif // MAINWINDOW_H
