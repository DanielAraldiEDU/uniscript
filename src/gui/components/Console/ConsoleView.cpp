#include "ConsoleView.h"

#include <QFont>
#include <QDateTime>
#include <QTextCursor>
#include <QTextCharFormat>

ConsoleView::ConsoleView(QWidget* parent) : QPlainTextEdit(parent) {
  QFont monoFont;
  monoFont.setFamily("Monospace");
  monoFont.setStyleHint(QFont::TypeWriter);
  monoFont.setPointSize(14);
  setFont(monoFont);
  setReadOnly(true);
  setMaximumBlockCount(1000);
}

void ConsoleView::appendLog(const QString& text, const QColor& color) {
  auto cursor = textCursor();
  cursor.movePosition(QTextCursor::End);
  cursor.insertText(QDateTime::currentDateTime().toString("[HH:mm:ss] "));
  QTextCharFormat cf; cf.setForeground(color);
  cursor.insertText(text + "\n", cf);
  setTextCursor(cursor);
  ensureCursorVisible();
}

