#pragma once

#include <QPlainTextEdit>
#include <QColor>

class ConsoleView : public QPlainTextEdit {
public:
  explicit ConsoleView(QWidget* parent = nullptr);
  void appendLog(const QString& text, const QColor& color);
};

