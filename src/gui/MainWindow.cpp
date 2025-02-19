// src/gui/MainWindow.cpp
#include "MainWindow.h"
#include <QApplication>
#include <QMenuBar>
#include <QMessageBox>
#include <QFileDialog>
#include <QDockWidget>
#include <QSettings>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle(tr("Chess Game"));
    
    // Create central widget
    boardView_ = new ChessBoardView(this);
    setCentralWidget(boardView_);
    
    // Create dock widgets
    controlPanel_ = new GameControlPanel(this);
    moveHistory_ = new MoveHistoryWidget(this);
    
    controlDock_ = new QDockWidget(tr("Game Control"), this);
    controlDock_->setWidget(controlPanel_);
    addDockWidget(Qt::RightDockWidgetArea, controlDock_);
    
    historyDock_ = new QDockWidget(tr("Move History"), this);
    historyDock_->setWidget(moveHistory_);
    addDockWidget(Qt::RightDockWidgetArea, historyDock_);
    
    createMenus();
    createToolBars();
    loadSettings();
    
    // Connect signals/slots
    connect(controlPanel_, &GameControlPanel::newGameRequested,
            this, &MainWindow::newGame);
    connect(boardView_, &ChessBoardView::moveCompleted,
            moveHistory_, &MoveHistoryWidget::addMove);
    connect(&ThemeManager::getInstance(), &ThemeManager::themeChanged,
            boardView_, &ChessBoardView::updateTheme);
}

void MainWindow::createMenus()
{
    QMenu* fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(tr("&New Game"), this, &MainWindow::newGame,
                       QKeySequence::New);
    fileMenu->addAction(tr("&Connect..."), this, &MainWindow::connectToGame);
    fileMenu->addSeparator();
    fileMenu->addAction(tr("&Save Game"), this, &MainWindow::saveGame,
                       QKeySequence::Save);
    fileMenu->addAction(tr("&Load Game"), this, &MainWindow::loadGame,
                       QKeySequence::Open);
    fileMenu->addSeparator();
    fileMenu->addAction(tr("&Exit"), qApp, &QApplication::quit,
                       QKeySequence::Quit);
    
    QMenu* viewMenu = menuBar()->addMenu(tr("&View"));
    viewMenu->addAction(controlDock_->toggleViewAction());
    viewMenu->addAction(historyDock_->toggleViewAction());
    
    QMenu* settingsMenu = menuBar()->addMenu(tr("&Settings"));
    settingsMenu->addAction(tr("&Preferences"), this, &MainWindow::showSettings);
    
    QMenu* helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(tr("&About"), this, &MainWindow::about);
}

void MainWindow::newGame()
{
    QMessageBox::StandardButton reply = QMessageBox::question(
        this, tr("New Game"),
        tr("Are you sure you want to start a new game?"),
        QMessageBox::Yes | QMessageBox::No);
        
    if (reply == QMessageBox::Yes) {
        boardView_->resetGame();
        moveHistory_->clear();
        controlPanel_->resetClock();
    }
}

void MainWindow::connectToGame()
{
    ConnectDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        // Handle connection
        QString address = dialog.getAddress();
        int port = dialog.getPort();
        
        try {
            boardView_->connectToServer(address, port);
            statusBar()->showMessage(tr("Connected to server"));
        } catch (const std::exception& e) {
            QMessageBox::critical(this, tr("Connection Error"),
                tr("Failed to connect: %1").arg(e.what()));
        }
    }
}

void MainWindow::saveGame()
{
    QString fileName = QFileDialog::getSaveFileName(this,
        tr("Save Game"), QString(),
        tr("Chess Game (*.pgn);;All Files (*)"));
        
    if (fileName.isEmpty()) {
        return;
    }
    
    try {
        boardView_->saveGame(fileName);
        statusBar()->showMessage(tr("Game saved"), 2000);
    } catch (const std::exception& e) {
        QMessageBox::critical(this, tr("Save Error"),
            tr("Failed to save game: %1").arg(e.what()));
    }
}

void MainWindow::loadGame()
{
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("Load Game"), QString(),
        tr("Chess Game (*.pgn);;All Files (*)"));
        
    if (fileName.isEmpty()) {
        return;
    }
    
    try {
        boardView_->loadGame(fileName);
        moveHistory_->loadFromGame(boardView_->getGame());
        statusBar()->showMessage(tr("Game loaded"), 2000);
    } catch (const std::exception& e) {
        QMessageBox::critical(this, tr("Load Error"),
            tr("Failed to load game: %1").arg(e.what()));
    }
}

void MainWindow::showSettings()
{
    SettingsDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        loadSettings();
        boardView_->applySettings();
        controlPanel_->applySettings();
    }
}

void MainWindow::about()
{
    QMessageBox::about(this, tr("About Chess Game"),
        tr("Chess Game v1.0\n\n"
           "A multiplayer chess game implemented with Qt.\n"
           "Licensed under GPL v3."));
}

void MainWindow::createToolBars()
{
    QToolBar* gameToolBar = addToolBar(tr("Game"));
    gameToolBar->addAction(QIcon(":/icons/new.png"), tr("New Game"),
                          this, &MainWindow::newGame);
    gameToolBar->addAction(QIcon(":/icons/connect.png"), tr("Connect"),
                          this, &MainWindow::connectToGame);
    gameToolBar->addAction(QIcon(":/icons/save.png"), tr("Save"),
                          this, &MainWindow::saveGame);
    gameToolBar->addAction(QIcon(":/icons/load.png"), tr("Load"),
                          this, &MainWindow::loadGame);
}

void MainWindow::loadSettings()
{
    QSettings settings;
    
    // Restore window geometry
    restoreGeometry(settings.value("geometry").toByteArray());
    restoreState(settings.value("windowState").toByteArray());
    
    // Load theme settings
    QString theme = settings.value("theme", "classic").toString();
    boardView_->setTheme(theme);
    
    // Load sound settings
    bool soundEnabled = settings.value("sound/enabled", true).toBool();
    int volume = settings.value("sound/volume", 100).toInt();
    controlPanel_->setSoundSettings(soundEnabled, volume);
}

void MainWindow::saveSettings()
{
    QSettings settings;
    
    // Save window geometry
    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());
    
    // Save other settings
    settings.setValue("theme", boardView_->getCurrentTheme());
    settings.setValue("sound/enabled", controlPanel_->isSoundEnabled());
    settings.setValue("sound/volume", controlPanel_->getVolume());
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    saveSettings();
    event->accept();
}