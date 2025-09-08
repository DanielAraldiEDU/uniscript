#include "HeaderBar.h"

#include <QHBoxLayout>

HeaderBar::HeaderBar(QWidget* parent) : QWidget(parent) {
  auto* layout = new QHBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(8);

  titleLabel = new QLabel(QStringLiteral("UniScript"), this);
  QFont titleFont = titleLabel->font();
  titleFont.setPointSize(14);
  titleFont.setBold(true);
  titleLabel->setFont(titleFont);

  btnCompile = new QPushButton(QStringLiteral("Compilar"), this);

  layout->addWidget(titleLabel);
  layout->addStretch();
  layout->addWidget(btnCompile);
}

