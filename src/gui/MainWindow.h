#pragma once

#include <QMainWindow>
#include <QStatusBar>
#include <QSplitter>

#include "components/Header/HeaderBar.h"
#include "components/Editor/CodeEditor.h"
#include "components/Console/ConsoleView.h"

class MainWindow : public QMainWindow {
public:
  explicit MainWindow(QWidget* parent = nullptr);

private:
  void compileSource();

  void applyDarkTheme();
  
  CodeEditor* editor;
  ConsoleView* console;
  HeaderBar* header;
};
