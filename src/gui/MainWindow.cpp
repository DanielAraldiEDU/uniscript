#include "MainWindow.h"

#include <QApplication>
#include <QFont>
#include <QLabel>
#include <QPalette>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDateTime>
#include <QAction>
#include <QTextCursor>
#include <QTextCharFormat>
#include <QTextOption>

#include "components/Header/HeaderBar.h"
#include "components/Editor/CodeEditor.h"
#include "components/Console/ConsoleView.h"

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

  auto* splitter = new QSplitter(Qt::Vertical, this);
  splitter->addWidget(editor);
  splitter->addWidget(console);
  splitter->setStretchFactor(0, 3);
  splitter->setStretchFactor(1, 1);

  vbox->addWidget(header);
  vbox->addWidget(splitter, 1);
  setCentralWidget(central);

  statusBar()->showMessage(QStringLiteral("Pronto"));

  cursorPosLabel = new QLabel(this);
  cursorPosLabel->setText(QStringLiteral("Linha 1, Coluna 1"));
  statusBar()->addPermanentWidget(cursorPosLabel);

  editor->setPlainText("while (x < 10){read(x);}\n");


  connect(editor, &QPlainTextEdit::cursorPositionChanged, this, [this]() {
    const auto cursor = editor->textCursor();
    const int line = cursor.blockNumber() + 1;
    const int col = cursor.positionInBlock() + 1;
    cursorPosLabel->setText(QString("Linha %1, Coluna %2").arg(line).arg(col));
  });

  // Contador de linha e coluna baseado no cursor/aonde o usuário está digitando no editor
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
  console->appendLog(QStringLiteral("Iniciando análise sintática…"), QColor("#60a5fa")); // blue-400

  const QString source = editor->toPlainText();
  if (source.trimmed().isEmpty()) {
    console->appendLog(QStringLiteral("Nenhum código para compilar."), QColor("#f87171")); // red-400
    statusBar()->showMessage(QStringLiteral("Falha: fonte vazia"));
    return;
  }

  try {
    Lexico lex;
    Sintatico sint;
    Semantico sem;

    std::string srcStd = source.toStdString();
    lex.setInput(srcStd.c_str());

    sint.parse(&lex, &sem);
    console->appendLog(QStringLiteral("Análise concluída com sucesso!"), QColor("#34d399"));
    statusBar()->showMessage(QStringLiteral("Compilado com sucesso"));
  } catch (const LexicalError& err) {
    console->appendLog(QString("Problema léxico: %1").arg(QString::fromUtf8(err.getMessage())), QColor("#f87171"));
    statusBar()->showMessage(QStringLiteral("Erro léxico"));
  } catch (const SyntacticError& err) {
    console->appendLog(QString("Problema sintático: %1").arg(QString::fromUtf8(err.getMessage())), QColor("#f87171"));
    statusBar()->showMessage(QStringLiteral("Erro sintático"));
  } catch (const SemanticError& err) {
    console->appendLog(QString("Problema semântico: %1").arg(QString::fromUtf8(err.getMessage())), QColor("#f87171"));
    statusBar()->showMessage(QStringLiteral("Erro semântico"));
  } catch (...) {
    console->appendLog(QStringLiteral("Erro desconhecido durante a compilação."), QColor("#f87171"));
    statusBar()->showMessage(QStringLiteral("Erro desconhecido"));
  }
}
