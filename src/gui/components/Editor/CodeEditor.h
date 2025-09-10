#pragma once

#include <QPlainTextEdit>
#include <QWidget>

class LineNumberArea;

class CodeEditor : public QPlainTextEdit {
public:
  explicit CodeEditor(QWidget* parent = nullptr);

  int lineNumberAreaWidth() const;
  void lineNumberAreaPaintEvent(QPaintEvent* event);
  void setMinGutterDigits(int digits) { minGutterDigits = qMax(1, digits); updateLineNumberAreaWidth(0); }

protected:
  void resizeEvent(QResizeEvent* event) override;

private:
  void updateLineNumberAreaWidth(int);
  void updateLineNumberArea(const QRect& rect, int dy);

  LineNumberArea* lineNumberArea;
  int minGutterDigits = 3; 
};

class LineNumberArea : public QWidget {
public:
  explicit LineNumberArea(CodeEditor* editor) : QWidget(editor), codeEditor(editor) {}
  QSize sizeHint() const override { return QSize(codeEditor->lineNumberAreaWidth(), 0); }

protected:
  void paintEvent(QPaintEvent* event) override { codeEditor->lineNumberAreaPaintEvent(event); }

private:
  CodeEditor* codeEditor;
};
