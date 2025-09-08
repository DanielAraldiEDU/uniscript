#include "CodeEditor.h"

#include <QFont>
#include <QTextOption>

CodeEditor::CodeEditor(QWidget* parent) : QPlainTextEdit(parent) {
  QFont monoFont;
  monoFont.setFamily("Monospace");
  monoFont.setStyleHint(QFont::TypeWriter);
  monoFont.setPointSize(14);
  setFont(monoFont);
  setPlaceholderText(QStringLiteral("Digite seu programa aqui…"));
  setWordWrapMode(QTextOption::NoWrap);
}

