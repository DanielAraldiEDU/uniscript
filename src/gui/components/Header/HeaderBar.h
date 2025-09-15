#pragma once

#include <QWidget>
#include <QPushButton>
#include <QLabel>

class HeaderBar : public QWidget {
public:
  explicit HeaderBar(QWidget* parent = nullptr);
  QPushButton* compileButton() const { return btnCompile; }

private:
  QLabel* titleLabel;
  QPushButton* btnCompile;
};

