// src/gui/MainWindow.cpp
#include "MainWindow.h"
#include "../util/ThemeManager.h"
#include "ConnectDialog.h"
#include <QtWidgets/QApplication>   // QApplication is main widget application class
#include <QtWidgets/QMenuBar>       // QMenuBar is a widget
#include <QtWidgets/QMessageBox>    // QMessageBox is a widget
#include <QtWidgets/QFileDialog>    // QFileDialog is a widget
#include <QtWidgets/QDockWidget>    // QDockWidget is a widget
#include <QtCore/QSettings>         // QSettings is core functionality

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle(tr("Multiplayer Chess"));
    
    // Create network client
    NetworkClient* networkClient = new NetworkClient(this);
    
    // Create central widget with network client
    boardView_ = new ChessBoardView(this);
    boardView_->setNetworkClient(networkClient);
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
            
    // Connect network signals
    connect(networkClient, &NetworkClient::connected,
            this, &MainWindow::onNetworkConnected);
    connect(networkClient, &NetworkClient::disconnected,
            this, &MainWindow::onNetworkDisconnected);
    connect(networkClient, &NetworkClient::errorOccurred,
            this, &MainWindow::onNetworkError);
}

void MainWindow::createMenus()
{
    QMenu* fileMenu = menuBar()->addMenu(tr("&File"));

    fileMenu->addAction(tr("&New Game"), QKeySequence::New, this, &MainWindow::newGame);
    fileMenu->addAction(tr("&Connect..."), this, &MainWindow::connectToGame);
    fileMenu->addSeparator();
    fileMenu->addAction(tr("&Save Game"), QKeySequence::Save, this, &MainWindow::saveGame);
    fileMenu->addAction(tr("&Load Game"), QKeySequence::Open, this, &MainWindow::loadGame);
    fileMenu->addSeparator();
    fileMenu->addAction(tr("&Exit"), QKeySequence::Quit, qApp, &QApplication::quit);
    
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
        QString serverAddress = dialog.getServerAddress();
        int port = dialog.getServerPort();
        
        if (boardView_->getNetworkClient()) {
            bool connected = boardView_->getNetworkClient()->connectToServer(serverAddress, port);
            if (connected) {
                statusBar()->showMessage(tr("Connected to %1:%2").arg(serverAddress).arg(port));
            } else {
                statusBar()->showMessage(tr("Failed to connect to %1:%2").arg(serverAddress).arg(port));
            }
        } else {
            statusBar()->showMessage(tr("Network client not initialized"));
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

MainWindow::~MainWindow()
{
    // Save settings on exit
    saveSettings();

    // Clean up owned widgets and objects
    // Most Qt widgets will be automatically deleted when their parent is deleted
    // We only need to manually delete objects that were created with 'new' and not parented

    // If you have any manually allocated objects that aren't parented, delete them here
    // For example:
    // delete networkManager_;

    // The MainWindow is the parent of most widgets, so they'll be automatically deleted
    // We only need to explicitly delete objects that don't have the MainWindow as parent
    
    // No need to delete these as they have MainWindow as parent:
    // boardView_, controlPanel_, moveHistory_, chatWidget_
    
    // Checking the codebase, there don't appear to be any non-parented objects 
    // that need manual deletion in MainWindow
}

void MainWindow::handleThemeChanged(const QString& theme)
{
    // Apply the new theme to the chess board
    if (boardView_) {
        boardView_->setTheme(theme);
    }

    // Update game control panel if it has theme-dependent elements
    if (controlPanel_) {
        controlPanel_->applySettings();
    }

    // Update any other UI elements that might have theme-dependent styling
    // For example, update status bar colors, menu styling, etc.

    // Update move history widget if it has theme-dependent styling
    if (moveHistory_) {
        // Your code doesn't seem to have a direct theme update method for moveHistory_
        // but if you add one in the future, call it here
    }

    // Save the theme in settings
    Settings& settings = Settings::getInstance();
    settings.setCurrentTheme(theme);
    settings.saveSettings();

    // Update window title to reflect theme if needed
    setWindowTitle(QString("Chess - %1").arg(theme));
    
    // Refresh the view
    update();
}

void MainWindow::onNetworkConnected()
{
    statusBar()->showMessage(tr("Connected to server"));
}

void MainWindow::onNetworkDisconnected()
{
    statusBar()->showMessage(tr("Disconnected from server"));
}

void MainWindow::onNetworkError(const QString& error)
{
    statusBar()->showMessage(tr("Network error: %1").arg(error));
}