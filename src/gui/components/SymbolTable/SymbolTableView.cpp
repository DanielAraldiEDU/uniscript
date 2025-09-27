#include "SymbolTableView.h"

#include <QAbstractItemView>
#include <QHeaderView>
#include <QTableWidgetItem>
#include <QtGlobal>

#include <cstddef>

namespace {
    QString toDisplay(const std::string& value) {
        return value.empty() ? QStringLiteral("-") : QString::fromStdString(value);
    }

    QString boolLabel(bool value) {
        return value ? QStringLiteral("Sim") : QStringLiteral("Nao");
    }
}

SymbolTableView::SymbolTableView(QWidget* parent)
    : QTableWidget(parent)
{
    setColumnCount(7);
    setHorizontalHeaderLabels({
        QStringLiteral("Identificador"),
        QStringLiteral("Tipo"),
        QStringLiteral("Modalidade"),
        QStringLiteral("Escopo"),
        QStringLiteral("Linha decl."),
        QStringLiteral("Inicializado"),
        QStringLiteral("Usado")
    });

    horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    verticalHeader()->setVisible(false);

    setSelectionMode(QAbstractItemView::SingleSelection);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    setAlternatingRowColors(true);
    setFocusPolicy(Qt::NoFocus);

#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    setPlaceholderText(QStringLiteral("Tabela vazia. Compile para visualizar os simbolos."));
#endif
}

void SymbolTableView::setSymbols(const std::vector<SymbolInfo>& symbols)
{
    clearContents();
    setRowCount(static_cast<int>(symbols.size()));

    for (int row = 0; row < static_cast<int>(symbols.size()); ++row) {
        const auto& symbol = symbols[static_cast<std::size_t>(row)];

        auto setItemText = [&](int column, const QString& text) {
            auto* item = new QTableWidgetItem(text);
            item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
            setItem(row, column, item);
        };

        setItemText(0, toDisplay(symbol.identifier));
        setItemText(1, toDisplay(symbol.type));
        setItemText(2, toDisplay(symbol.modality));
        setItemText(3, toDisplay(symbol.scope));
        setItemText(4, symbol.declaredLine > 0 ? QString::number(symbol.declaredLine) : QStringLiteral("-"));
        setItemText(5, boolLabel(symbol.initialized));
        setItemText(6, boolLabel(symbol.used));
    }
}

void SymbolTableView::clearSymbols()
{
    clearContents();
    setRowCount(0);
}
