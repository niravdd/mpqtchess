// src/gui/MainWindow.h
#pragma once
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QStatusBar>
#include "ChessBoardView.h"
#include "GameControlPanel.h"
#include "MoveHistoryWidget.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

private slots:
    void newGame();
    void connectToGame();
    void saveGame();
    void loadGame();
    void showSettings();
    void about();

private:
    void createMenus();
    void createToolBars();
    void setupLayout();
    void loadSettings();
    void saveSettings();

    ChessBoardView* boardView_;
    GameControlPanel* controlPanel_;
    MoveHistoryWidget* moveHistory_;
    QDockWidget* controlDock_;
    QDockWidget* historyDock_;
};