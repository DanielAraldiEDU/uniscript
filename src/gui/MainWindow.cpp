#include "MainWindow.h"

#include <QApplication>
#include <QFont>
#include <QLabel>
#include <QPalette>
#include <QVBoxLayout>
#include <QDateTime>
#include <QAction>
#include <QTextCursor>
#include <QTextCharFormat>
#include <QTextOption>
#include <QSplitter>

#include "components/Header/HeaderBar.h"
#include "components/Editor/CodeEditor.h"
#include "components/Console/ConsoleView.h"
#include "components/SymbolTable/SymbolTableView.h"

#include "gals/Lexico.h"
#include "gals/Sintatico.h"
#include "gals/Semantico.h"
#include "gals/LexicalError.h"
#include "gals/SyntacticError.h"
#include "gals/SemanticError.h"

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
  applyDarkTheme();

  auto* central = new QWidget(this);
  auto* vbox = new QVBoxLayout(central);

  header = new HeaderBar(this);
  editor = new CodeEditor(this);
  console = new ConsoleView(this);
  tableView = new SymbolTableView(this);
  tableView->setMinimumWidth(280);

  auto* editorAndTable = new QSplitter(Qt::Horizontal, this);
  editorAndTable->addWidget(editor);
  editorAndTable->addWidget(tableView);
  editorAndTable->setStretchFactor(0, 3);
  editorAndTable->setStretchFactor(1, 2);

  auto* mainSplitter = new QSplitter(Qt::Vertical, this);
  mainSplitter->addWidget(editorAndTable);
  mainSplitter->addWidget(console);
  mainSplitter->setStretchFactor(0, 3);
  mainSplitter->setStretchFactor(1, 1);

  vbox->addWidget(header);
  vbox->addWidget(mainSplitter, 1);
  setCentralWidget(central);

  statusBar()->showMessage(QStringLiteral("Pronto"));

  cursorPosLabel = new QLabel(this);
  cursorPosLabel->setText(QStringLiteral("Linha 1, Coluna 1"));
  statusBar()->addPermanentWidget(cursorPosLabel);

  editor->setPlainText("print(\"Hello, World!\");");

  connect(editor, &QPlainTextEdit::cursorPositionChanged, this, [this]() {
    const auto cursor = editor->textCursor();
    const int line = cursor.blockNumber() + 1;
    const int col = cursor.positionInBlock() + 1;
    cursorPosLabel->setText(QString("Linha %1, Coluna %2").arg(line).arg(col));
  });

  // Atualiza rotulo inicial do cursor apos configurar o editor
  QMetaObject::invokeMethod(editor, [this]() {
    const auto cursor = editor->textCursor();
    const int line = cursor.blockNumber() + 1;
    const int col = cursor.positionInBlock() + 1;
    cursorPosLabel->setText(QString("Linha %1, Coluna %2").arg(line).arg(col));
  }, Qt::QueuedConnection);

  connect(header->compileButton(), &QPushButton::clicked, this, &MainWindow::compileSource);
}

void MainWindow::applyDarkTheme() {
  QPalette pal;
  pal.setColor(QPalette::Window, QColor("#09090b"));
  pal.setColor(QPalette::WindowText, QColor("#e4e4e7"));
  pal.setColor(QPalette::Base, QColor("#0b0b0f"));
  pal.setColor(QPalette::AlternateBase, QColor("#18181b"));
  pal.setColor(QPalette::Text, QColor("#e4e4e7"));
  pal.setColor(QPalette::ToolTipBase, QColor("#18181b"));
  pal.setColor(QPalette::ToolTipText, QColor("#fafafa"));
  pal.setColor(QPalette::Button, QColor("#18181b"));
  pal.setColor(QPalette::ButtonText, QColor("#e4e4e7"));
  pal.setColor(QPalette::BrightText, QColor("#ffffff"));
  pal.setColor(QPalette::Highlight, QColor("#3b82f6"));
  pal.setColor(QPalette::HighlightedText, QColor("#0a0a0a"));
  qApp->setPalette(pal);

  qApp->setStyleSheet(R"( 
    QMainWindow { background-color: #09090b; }
    QToolBar { background-color: #18181b; border: none; padding: 6px; }
    QToolButton { color: #e4e4e7; padding: 6px 10px; }
    QLabel { color: #e4e4e7; }
    QPushButton { 
      color: #e4e4e7; background-color: #27272a; 
      border: 1px solid #3f3f46; border-radius: 8px; padding: 8px 14px; 
    }
    QPushButton:hover { background-color: #303036; }
    QPushButton:pressed { background-color: #1f2937; }
    QPlainTextEdit { 
      background-color: #0b0b0f; color: #e4e4e7; 
      border: 1px solid #27272a; border-radius: 8px; padding: 8px 8px 8px 0px; 
      selection-background-color: #3b82f6; selection-color: #0a0a0a;
    }
    QStatusBar { color: #a1a1aa; background-color: #18181b; }
  )");
}

void MainWindow::compileSource() {
  console->clear();
  editor->clearErrorMarkers();
  tableView->clearSymbols();
  console->appendLog(QStringLiteral("Iniciando analise sintetica..."), QColor("#60a5fa"));

  const QString source = editor->toPlainText();
  if (source.trimmed().isEmpty()) {
    console->appendLog(QStringLiteral("Nenhum codigo para compilar."), QColor("#f87171"));
    statusBar()->showMessage(QStringLiteral("Falha: fonte vazia"));
    return;
  }

  Lexico lex;
  Sintatico sint;
  Semantico sem;
  sem.clearSymbolTable();

  std::string srcStd = source.toStdString();
  lex.setInput(srcStd.c_str());
  sem.setSourceCode(srcStd);

  auto updateTable = [&]() {
    tableView->setSymbols(sem.symbolTable());
  };

  try {
    sint.parse(&lex, &sem);
    updateTable();
    console->appendLog(QStringLiteral("Analise concluida com sucesso!"), QColor("#34d399"));
    statusBar()->showMessage(QStringLiteral("Compilado com sucesso"));
  } catch (const LexicalError& err) {
    updateTable();
    const int pos = err.getPosition();
    int lineNo = -1, colNo = -1;
    if (pos >= 0) {
      QTextCursor tc(editor->document());
      const int docChars = editor->document()->characterCount();
      tc.setPosition(qBound(0, pos, docChars - 1));
      lineNo = tc.blockNumber() + 1;
      colNo = tc.positionInBlock() + 1;
    }
    const auto msg = (lineNo > 0)
      ? QString("Problema lexico (linha %1, coluna %2): %3").arg(lineNo).arg(colNo).arg(QString::fromUtf8(err.getMessage()))
      : QString("Problema lexico: %1").arg(QString::fromUtf8(err.getMessage()));
    console->appendLog(msg, QColor("#f87171"));
    if (pos >= 0) {
      int start = pos;
      int end = pos;
      auto isWordChar = [&](QChar ch){ return ch.isLetterOrNumber() || ch == QChar('_'); };
      while (start > 0 && isWordChar(source.at(start - 1))) --start;
      while (end < source.size() && isWordChar(source.at(end))) ++end;
      const int len = qMax(1, end - start);
      editor->showErrorAt(start, len);
    }
    statusBar()->showMessage((lineNo > 0)
      ? QString("Erro lexico na linha %1, coluna %2").arg(lineNo).arg(colNo)
      : QStringLiteral("Erro lexico"));
  } catch (const SyntacticError& err) {
    updateTable();
    const int pos = err.getPosition();
    int lineNo = -1, colNo = -1;
    if (pos >= 0) {
      QTextCursor tc(editor->document());
      const int docChars = editor->document()->characterCount();
      tc.setPosition(qBound(0, pos, docChars - 1));
      lineNo = tc.blockNumber() + 1;
      colNo = tc.positionInBlock() + 1;
    }
    const auto msg = (lineNo > 0)
      ? QString("Problema sintetico (linha %1, coluna %2): %3").arg(lineNo).arg(colNo).arg(QString::fromUtf8(err.getMessage()))
      : QString("Problema sintetico: %1").arg(QString::fromUtf8(err.getMessage()));
    console->appendLog(msg, QColor("#f87171"));
    if (pos >= 0) {
      int start = pos;
      int end = pos;
      auto isWordChar = [&](QChar ch){ return ch.isLetterOrNumber() || ch == QChar('_'); };
      while (start > 0 && isWordChar(source.at(start - 1))) --start;
      while (end < source.size() && isWordChar(source.at(end))) ++end;
      const int len = qMax(1, end - start);
      editor->showErrorAt(start, len);
    }
    statusBar()->showMessage((lineNo > 0)
      ? QString("Erro sintetico na linha %1, coluna %2").arg(lineNo).arg(colNo)
      : QStringLiteral("Erro sintetico"));
  } catch (const SemanticError& err) {
    updateTable();
    const int pos = err.getPosition();
    int lineNo = -1, colNo = -1;
    if (pos >= 0) {
      QTextCursor tc(editor->document());
      const int docChars = editor->document()->characterCount();
      tc.setPosition(qBound(0, pos, docChars - 1));
      lineNo = tc.blockNumber() + 1;
      colNo = tc.positionInBlock() + 1;
    }
    const auto msg = (lineNo > 0)
      ? QString("Problema semantico (linha %1, coluna %2): %3").arg(lineNo).arg(colNo).arg(QString::fromUtf8(err.getMessage()))
      : QString("Problema semantico: %1").arg(QString::fromUtf8(err.getMessage()));
    console->appendLog(msg, QColor("#f87171"));
    if (pos >= 0) {
      int start = pos;
      int end = pos;
      auto isWordChar = [&](QChar ch){ return ch.isLetterOrNumber() || ch == QChar('_'); };
      while (start > 0 && isWordChar(source.at(start - 1))) --start;
      while (end < source.size() && isWordChar(source.at(end))) ++end;
      const int len = qMax(1, end - start);
      editor->showErrorAt(start, len);
    }
    statusBar()->showMessage((lineNo > 0)
      ? QString("Erro semantico na linha %1, coluna %2").arg(lineNo).arg(colNo)
      : QStringLiteral("Erro semantico"));
  } catch (...) {
    updateTable();
    console->appendLog(QStringLiteral("Erro desconhecido durante a compilacao."), QColor("#f87171"));
    statusBar()->showMessage(QStringLiteral("Erro desconhecido"));
  }
}
