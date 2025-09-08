#pragma once

#include <QPlainTextEdit>

class CodeEditor : public QPlainTextEdit {
public:
  explicit CodeEditor(QWidget* parent = nullptr);
};

