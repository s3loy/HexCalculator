#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QPushButton>
#include <QRegularExpression>

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
        b->setFocusPolicy(Qt::NoFocus);
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

    CalculatorCore::Result res = m_calc.compute(expr);

    ui->exprLineEdit->setText(expr);
    ui->resultLineEdit->setText(res.valueStr);

    ui->exprLineEdit->setFocus();
    ui->exprLineEdit->setCursorPosition(ui->exprLineEdit->text().length());
}
