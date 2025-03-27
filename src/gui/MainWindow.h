// src/gui/MainWindow.h
#pragma once
#include <memory>
#include "../core/ChessGame.h"
#include "ChessBoardView.h"
#include "GameControlPanel.h"
#include "MoveHistoryWidget.h"
#include "ConnectDialog.h"
#include "SettingsDialog.h"
#include "../network/ChessClient.h"
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QToolBar>
#include <QtGui/QCloseEvent>
#include <QtGui/QIcon>

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

    // Disable copying
    MainWindow(const MainWindow&) = delete;
    MainWindow& operator=(const MainWindow&) = delete;

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void newGame();
    void connectToGame();
    void saveGame();
    void loadGame();
    void showSettings();
    void about();
    void handleThemeChanged(const QString& theme);

private:
    void createMenus();
    void createToolBars();
//  void setupLayout();
    void loadSettings();
    void saveSettings();

    std::unique_ptr<ChessGame> game_;
    std::unique_ptr<ChessClient> client_;
    ChessBoardView* boardView_;
    GameControlPanel* controlPanel_;
    MoveHistoryWidget* moveHistory_;
    QDockWidget* controlDock_;
    QDockWidget* historyDock_;
};