#pragma once

#include <QTableWidget>

#include <vector>

#include "gals/SymbolInfo.h"

class SymbolTableView : public QTableWidget {
public:
    explicit SymbolTableView(QWidget* parent = nullptr);

    void setSymbols(const std::vector<SymbolInfo>& symbols);
    void clearSymbols();
};
