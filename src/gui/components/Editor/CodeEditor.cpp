#include "CodeEditor.h"

#include <QFont>
#include <QPainter>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QTextBlock>
#include <QTextOption>

CodeEditor::CodeEditor(QWidget* parent) : QPlainTextEdit(parent) {
  QFont monoFont;
  monoFont.setFamily("Monospace");
  monoFont.setStyleHint(QFont::TypeWriter);
  monoFont.setPointSize(14);
  setFont(monoFont);
  setPlaceholderText(QStringLiteral("Digite seu programa aqui…"));
  setWordWrapMode(QTextOption::NoWrap);
  
  lineNumberArea = new LineNumberArea(this);

  connect(this, &CodeEditor::blockCountChanged, this, &CodeEditor::updateLineNumberAreaWidth);
  connect(this, &CodeEditor::updateRequest, this, &CodeEditor::updateLineNumberArea);

  updateLineNumberAreaWidth(0);
}

int CodeEditor::lineNumberAreaWidth() const {
  int digits = 1;
  int max = qMax(1, blockCount());
  while (max >= 10) {
    max /= 10;
    ++digits;
  }
  const int usedDigits = qMax(minGutterDigits, digits);
  const int charW = fontMetrics().horizontalAdvance(QLatin1Char('9'));
  const int leftPad = 0;            // no extra left padding
  const int rightPad = charW;       // single-space separation between number and code
  const int space = leftPad + charW * usedDigits + rightPad;
  return space;
}

void CodeEditor::updateLineNumberAreaWidth(int) {
  setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void CodeEditor::updateLineNumberArea(const QRect& rect, int dy) {
  if (dy) {
    lineNumberArea->scroll(0, dy);
  } else {
    lineNumberArea->update(0, rect.y(), lineNumberArea->width(), rect.height());
  }

  if (rect.contains(viewport()->rect())) {
    updateLineNumberAreaWidth(0);
  }
}

void CodeEditor::resizeEvent(QResizeEvent* e) {
  QPlainTextEdit::resizeEvent(e);

  QRect cr = contentsRect();
  lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}

void CodeEditor::lineNumberAreaPaintEvent(QPaintEvent* event) {
  QPainter painter(lineNumberArea);

  // Colors matching the dark theme (VS Code-like gutter, no border)
  const QColor bg("#0b0b0f");
  const QColor fg("#a1a1aa");
  const QColor fgCurrent("#e4e4e7");

  painter.fillRect(event->rect(), bg);

  QTextBlock block = firstVisibleBlock();
  int blockNumber = block.blockNumber();
  const int topOffset = static_cast<int>(blockBoundingGeometry(block).translated(contentOffset()).top());
  int top = topOffset;
  int bottom = top + static_cast<int>(blockBoundingRect(block).height());

  const int sep = fontMetrics().horizontalAdvance(QLatin1Char(' '));
  const int right = lineNumberArea->width() - sep; // leave exactly one space to the code

  while (block.isValid() && top <= event->rect().bottom()) {
    if (block.isVisible() && bottom >= event->rect().top()) {
      const bool isCurrent = (textCursor().blockNumber() == blockNumber);
      const QString number = QString::number(blockNumber + 1);
      painter.setPen(isCurrent ? fgCurrent : fg);
      painter.drawText(0, top, right, fontMetrics().height(), Qt::AlignRight | Qt::AlignVCenter, number);
    }

    block = block.next();
    top = bottom;
    bottom = top + static_cast<int>(blockBoundingRect(block).height());
    ++blockNumber;
  }

  // No explicit separator line to match VS Code style
}
