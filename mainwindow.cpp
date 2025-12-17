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

    static const QString allowedChars = "0123456789ABCDEFabcdef.+-*/%^!&|~<>()=xX÷！（）《》～";

    QString filteredText;
    for (const QChar &c : text){
        if (allowedChars.contains(c) || c. isSpace()){
            filteredText += c;
        }
    }

    if (text.contains('=')){
        QString exprWithoutEqual = filteredText;
        exprWithoutEqual.remove('=');

        m_updatingText = true;
        ui->exprLineEdit->setText(normalizeExpression(exprWithoutEqual));
        ui->exprLineEdit->setCursorPosition(ui->exprLineEdit->text().length());
        m_updatingText = false;

        computeAndShow();
        return;
    }

    int cursor = ui->exprLineEdit->cursorPosition();

    int filteredCursor = 0;
    for (int i = 0; i < cursor && i < text.length(); i++){
        if (allowedChars.contains(text[i]) || text[i].isSpace()){
            filteredCursor++;
        }
    }
    cursor = filteredCursor;

    QString newText = filteredText;
    int newCursor = cursor;

    newText.replace("《", "<");
    newText.replace("》", ">");

    // 检测是否是删除操作
    static QString lastText = "";
    bool isDeleting = (newText.length() < lastText.length());

    if (isDeleting && cursor > 0 && cursor <= newText.length()){
        QChar charAtCursor = newText[cursor - 1];

        if (charAtCursor == '<'){
            bool hasAnotherLeft = (cursor >= 2 && newText[cursor - 2] == '<') ||
                                  (cursor < newText.length() && newText[cursor] == '<');
            if (!hasAnotherLeft){
                newText = newText.left(cursor - 1) + newText.mid(cursor);
                newCursor = cursor - 1;
            }
        }
        else if (charAtCursor == '>'){
            bool hasAnotherRight = (cursor >= 2 && newText[cursor - 2] == '>') ||
                                   (cursor < newText. length() && newText[cursor] == '>');
            if (!hasAnotherRight){
                newText = newText.left(cursor - 1) + newText.mid(cursor);
                newCursor = cursor - 1;
            }
        }
    }

    if (! isDeleting && cursor > 0 && cursor <= newText.length()){
        QChar lastChar = newText[cursor - 1];

        if (lastChar == '<'){
            bool prevIsLessThan = (cursor >= 2 && newText[cursor - 2] == '<');
            if (!prevIsLessThan){
                newText = newText.left(cursor) + "<" + newText.mid(cursor);
                newCursor = cursor + 1;
            }
        }
        else if (lastChar == '>'){
            bool prevIsGreaterThan = (cursor >= 2 && newText[cursor - 2] == '>');
            if (!prevIsGreaterThan){
                newText = newText.left(cursor) + ">" + newText.mid(cursor);
                newCursor = cursor + 1;
            }
        }
    }

    const QString normalized = normalizeExpression(newText);

    lastText = normalized;

    if (normalized == text && newText == text) return;

    m_updatingText = true;

    if (newText != filteredText){
        int extraSpaces = normalized.length() - newText.length();
        newCursor += extraSpaces;
        if (newCursor > normalized.length()) newCursor = normalized.length();
        if (newCursor < 0) newCursor = 0;
    } else {
        int charCountBeforeCursor = 0;
        for (int i = 0; i < cursor && i < filteredText.length(); i++){
            if (! filteredText[i].isSpace()){
                charCountBeforeCursor++;
            }
        }

        newCursor = 0;
        int count = 0;
        for (int i = 0; i < normalized.length(); i++){
            if (!normalized[i].isSpace()){
                count++;
            }
            if (count >= charCountBeforeCursor){
                newCursor = i + 1;
                break;
            }
        }

        if (count < charCountBeforeCursor){
            newCursor = normalized.length();
        }
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

    if (t == "&&AND"){
        insertToExpr("&");
        return;
    }
    if (t == "|OR"){
        insertToExpr("|");
        return;
    }
    if (t == "^XOR"){
        insertToExpr("^^");
        return;
    }
    if (t == "~NOT"){
        insertToExpr("~");
        return;
    }
    if (t == "^n"){
        insertToExpr("^");
        return;
    }
    static const QSet<QString> ops {"+","-","*","/","%","^","(",")","!","<<",">>"};
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
    QString text = ui->exprLineEdit->text();
    if (text.isEmpty()) return;

    while (text.endsWith(' ')){
        text.chop(1);
    }

    if (text.isEmpty()){
        ui->exprLineEdit->clear();
        return;
    }

    if (text.endsWith("<<") || text.endsWith(">>") || text.endsWith("^^")){
        text.chop(2);
    } else {
        text.chop(1);
    }

    while (text. endsWith(' ')){
        text. chop(1);
    }

    QString normalized = normalizeExpression(text);

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
    QString s = in. toUpper();

    s.replace(QRegularExpression("\\s+"), " ");
    s = s. trimmed();

    s. replace("x", "*");
    s.replace("÷", "/");
    s.replace("！", "!");
    s.replace("（", "(");
    s.replace("）", ")");
    s.replace("～", "~");

    while (s.contains("<<<")) s.replace("<<<", "<<");
    while (s.contains(">>>")) s.replace(">>>", ">>");

    s.replace(QRegularExpression("\\^\\s+\\^"), "^^");
    s.replace(QRegularExpression("<\\s+<"), "<<");
    s.replace(QRegularExpression(">\\s+>"), ">>");

    s. replace("^^", " ^^ ");
    s.replace("<<", " << ");
    s.replace(">>", " >> ");

    static const QRegularExpression singleOpRe("([+\\-*/()%!&|~])");
    s.replace(singleOpRe, " \\1 ");

    s. replace(QRegularExpression("(?<!\\^)\\^(?!\\^)"), " ^ ");

    s.replace(QRegularExpression("\\^\\s+\\^"), "^^");
    s.replace("^^", " ^^ ");

    s. replace(QRegularExpression("\\s+"), " ");
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
