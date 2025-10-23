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
    setColumnCount(10);
    setHorizontalHeaderLabels({
        QStringLiteral("Identificador"),
        QStringLiteral("Tipo"),
        QStringLiteral("Mutabilidade"),
        QStringLiteral("Inicializada"),
        QStringLiteral("Usada"),
        QStringLiteral("Escopo"),
        QStringLiteral("Posição"),
        QStringLiteral("Parâmetro"),
        QStringLiteral("Vetor"),
        QStringLiteral("Função")
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
        setItemText(2, symbol.isConstant ? QStringLiteral("Const") : QStringLiteral("Var"));
        setItemText(3, boolLabel(symbol.initialized));
        setItemText(4, boolLabel(symbol.used));
        setItemText(5, QString::number(symbol.scope));
        if (symbol.line > 0) {
            setItemText(6, QStringLiteral("%1:%2").arg(symbol.line).arg(symbol.column > 0 ? symbol.column : 1));
        } else {
            setItemText(6, QStringLiteral("-"));
        }
        setItemText(7, boolLabel(symbol.isParameter));
        setItemText(8, boolLabel(symbol.isArray));
        setItemText(9, boolLabel(symbol.isFunction));
    }
}

void SymbolTableView::clearSymbols()
{
    clearContents();
    setRowCount(0);
}
