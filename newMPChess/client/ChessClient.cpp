// ChessClient.cpp
#include "ChessClient.h"
#include "LoginDialog.h"
#include "RegistrationDialog.h"
#include "MatchmakingDialog.h"
#include "LeaderboardDialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QInputDialog>
#include <QSettings>
#include <QSplitter>
#include <QFileDialog>
#include <QApplication>
#include <QStyle>
#include <QSound>

ChessClient::ChessClient(QWidget* parent)
    : QMainWindow(parent),
      serverHost("localhost"),
      serverPort(8080),
      userRating(0),
      currentGameId(0),
      isWhitePlayer(false),
      isOpponentBot(false),
      whiteRemainingTime(0),
      blackRemainingTime(0),
      currentState(GameState::LOGIN)
{
    setupUI();
    createActions();
    createMenus();
    createToolbars();
    setupConnections();
    
    // Initialize dialogs
    loginDialog = new LoginDialog(this);
    registrationDialog = new RegistrationDialog(this);
    matchmakingDialog = new MatchmakingDialog(this);
    leaderboardDialog = new LeaderboardDialog(this);
    
    // Initialize sounds
    moveSound = new QSound(":/Resources/sounds/move.wav", this);
    captureSound = new QSound(":/Resources/sounds/capture.wav", this);
    checkSound = new QSound(":/Resources/sounds/check.wav", this);
    gameEndSound = new QSound(":/Resources/sounds/game_end.wav", this);
    
    // Initialize timer
    gameTimer = new QTimer(this);
    connect(gameTimer, &QTimer::timeout, this, &ChessClient::updateTimers);
    gameTimer->start(100); // Update every 100ms
    
    // Load settings
    loadSettings();
    
    // Start in login state
    setState(GameState::LOGIN);
    QTimer::singleShot(100, this, &ChessClient::showLogin);
}

ChessClient::~ChessClient() {
    saveSettings();
    
    if (NetworkManager::getInstance().isConnected()) {
        NetworkManager::getInstance().disconnectFromServer();
    }
}

void ChessClient::setupUI() {
    // Setup central widget with stacked layout
    centralStack = new QStackedWidget(this);
    setCentralWidget(centralStack);
    
    // Create main menu widget
    mainMenuWidget = new QWidget();
    QVBoxLayout* mainMenuLayout = new QVBoxLayout(mainMenuWidget);
    
    // Add fancy welcome message
    QLabel* welcomeLabel = new QLabel("Welcome to Chess Game");
    welcomeLabel->setStyleSheet("font-size: 24pt; font-weight: bold; color: #2c3e50;");
    welcomeLabel->setAlignment(Qt::AlignCenter);
    
    QPushButton* playButton = new QPushButton("Play Game");
    QPushButton* leaderboardButton = new QPushButton("Leaderboard");
    QPushButton* statsButton = new QPushButton("Player Statistics");
    QPushButton* settingsButton = new QPushButton("Settings");
    
    playButton->setMinimumHeight(50);
    leaderboardButton->setMinimumHeight(50);
    statsButton->setMinimumHeight(50);
    settingsButton->setMinimumHeight(50);
    
    mainMenuLayout->addWidget(welcomeLabel);
    mainMenuLayout->addSpacing(20);
    mainMenuLayout->addWidget(playButton);
    mainMenuLayout->addWidget(leaderboardButton);
    mainMenuLayout->addWidget(statsButton);
    mainMenuLayout->addWidget(settingsButton);
    mainMenuLayout->addStretch();
    
    connect(playButton, &QPushButton::clicked, this, &ChessClient::showMatchmaking);
    connect(leaderboardButton, &QPushButton::clicked, this, &ChessClient::showLeaderboard);
    connect(statsButton, &QPushButton::clicked, this, &ChessClient::showPlayerStats);
    
    // Create game widget
    gameWidget = new QWidget();
    QVBoxLayout* gameLayout = new QVBoxLayout(gameWidget);
    
    QSplitter* gameSplitter = new QSplitter(Qt::Horizontal);
    
    // Create chess board
    chessBoard = new ChessBoard();
    
    // Create side panel
    QWidget* sidePanel = new QWidget();
    QVBoxLayout* sidePanelLayout = new QVBoxLayout(sidePanel);
    
    playerInfoWidget = new PlayerInfoWidget();
    analysisWidget = new GameAnalysisWidget();
    
    QPushButton* recommendMoveBtn = new QPushButton("Get Move Recommendations");
    QPushButton* analyzeGameBtn = new QPushButton("Analyze Game");
    QPushButton* drawBtn = new QPushButton("Offer Draw");
    QPushButton* resignBtn = new QPushButton("Resign");
    
    sidePanelLayout->addWidget(playerInfoWidget);
    sidePanelLayout->addWidget(analysisWidget);
    sidePanelLayout->addWidget(recommendMoveBtn);
    sidePanelLayout->addWidget(analyzeGameBtn);
    sidePanelLayout->addWidget(drawBtn);
    sidePanelLayout->addWidget(resignBtn);
    sidePanelLayout->addStretch();
    
    connect(recommendMoveBtn, &QPushButton::clicked, this, &ChessClient::requestMoveRecommendations);
    connect(analyzeGameBtn, &QPushButton::clicked, this, &ChessClient::requestGameAnalysis);
    connect(drawBtn, &QPushButton::clicked, this, &ChessClient::requestDraw);
    connect(resignBtn, &QPushButton::clicked, this, &ChessClient::resignGame);
    
    gameSplitter->addWidget(chessBoard);
    gameSplitter->addWidget(sidePanel);
    gameSplitter->setStretchFactor(0, 3); // Board gets more space
    gameSplitter->setStretchFactor(1, 1); // Info panel gets less space
    
    gameLayout->addWidget(gameSplitter);
    
    // Add widgets to stacked widget
    centralStack->addWidget(mainMenuWidget);
    centralStack->addWidget(gameWidget);
    
    // Setup status bar
    statusLabel = new QLabel("Not connected");
    connectionLabel = new QLabel("Disconnected");
    connectionLabel->setStyleSheet("color: red;");
    
    statusBar()->addWidget(statusLabel, 1);
    statusBar()->addPermanentWidget(connectionLabel);
    
    // Set window properties
    setWindowTitle("Chess Client");
    resize(1024, 768);
}

void ChessClient::createActions() {
    connectAction = new QAction("Connect to Server", this);
    disconnectAction = new QAction("Disconnect from Server", this);
    loginAction = new QAction("Login", this);
    registerAction = new QAction("Register", this);
    matchmakingAction = new QAction("Find Game", this);
    leaderboardAction = new QAction("Leaderboard", this);
    playerStatsAction = new QAction("Player Statistics", this);
    drawAction = new QAction("Offer Draw", this);
    resignAction = new QAction("Resign", this);
    analysisAction = new QAction("Analyze Game", this);
    
    connect(connectAction, &QAction::triggered, this, &ChessClient::connectToServer);
    connect(disconnectAction, &QAction::triggered, this, &ChessClient::disconnectFromServer);
    connect(loginAction, &QAction::triggered, this, &ChessClient::showLogin);
    connect(registerAction, &QAction::triggered, this, &ChessClient::showRegistration);
    connect(matchmakingAction, &QAction::triggered, this, &ChessClient::showMatchmaking);
    connect(leaderboardAction, &QAction::triggered, this, &ChessClient::showLeaderboard);
    connect(playerStatsAction, &QAction::triggered, this, &ChessClient::showPlayerStats);
    connect(drawAction, &QAction::triggered, this, &ChessClient::requestDraw);
    connect(resignAction, &QAction::triggered, this, &ChessClient::resignGame);
    connect(analysisAction, &QAction::triggered, this, &ChessClient::requestGameAnalysis);
}

void ChessClient::createMenus() {
    QMenu* serverMenu = menuBar()->addMenu("Server");
    serverMenu->addAction(connectAction);
    serverMenu->addAction(disconnectAction);
    serverMenu->addSeparator();
    serverMenu->addAction(loginAction);
    serverMenu->addAction(registerAction);
    
    QMenu* gameMenu = menuBar()->addMenu("Game");
    gameMenu->addAction(matchmakingAction);
    gameMenu->addSeparator();
    gameMenu->addAction(drawAction);
    gameMenu->addAction(resignAction);
    gameMenu->addAction(analysisAction);
    
    QMenu* viewMenu = menuBar()->addMenu("View");
    viewMenu->addAction(leaderboardAction);
    viewMenu->addAction(playerStatsAction);
    
    QMenu* helpMenu = menuBar()->addMenu("Help");
    helpMenu->addAction("About", this, [this]() {
        QMessageBox::about(this, "About Chess Client",
                          "Chess Client v1.0\n\n"
                          "A beautiful chess game client that connects to the multiplayer chess server.");
    });
}

void ChessClient::createToolbars() {
    QToolBar* gameToolbar = addToolBar("Game");
    gameToolbar->addAction(matchmakingAction);
    gameToolbar->addAction(leaderboardAction);
    gameToolbar->addAction(playerStatsAction);
}

void ChessClient::setupConnections() {
    // Connect to network manager signals
    connect(&NetworkManager::getInstance(), &NetworkManager::connected, this, [this]() {
        connectionLabel->setText("Connected");
        connectionLabel->setStyleSheet("color: green;");
        statusLabel->setText("Connected to server");
    });
    
    connect(&NetworkManager::getInstance(), &NetworkManager::disconnected, this, [this]() {
        connectionLabel->setText("Disconnected");
        connectionLabel->setStyleSheet("color: red;");
        statusLabel->setText("Disconnected from server");
    });
    
    connect(&NetworkManager::getInstance(), &NetworkManager::errorOccurred, this, [this](const QString& error) {
        connectionLabel->setText("Error");
        connectionLabel->setStyleSheet("color: red;");
        statusLabel->setText("Error: " + error);
        
        QMessageBox::critical(this, "Connection Error", "Failed to connect to server: " + error);
    });
    
    connect(&NetworkManager::getInstance(), &NetworkManager::messageReceived, this, &ChessClient::handleNetworkMessage);
    
    // Connect chess board signals
    connect(chessBoard, &ChessBoard::moveMade, this, &ChessClient::makeBoardMove);
}

void ChessClient::setState(GameState state) {
    currentState = state;
    
    // Update UI based on state
    switch (state) {
        case GameState::LOGIN:
            centralStack->setCurrentWidget(mainMenuWidget);
            loginAction->setEnabled(true);
            registerAction->setEnabled(true);
            matchmakingAction->setEnabled(false);
            drawAction->setEnabled(false);
            resignAction->setEnabled(false);
            analysisAction->setEnabled(false);
            break;
            
        case GameState::MAIN_MENU:
            centralStack->setCurrentWidget(mainMenuWidget);
            loginAction->setEnabled(false);
            registerAction->setEnabled(false);
            matchmakingAction->setEnabled(true);
            drawAction->setEnabled(false);
            resignAction->setEnabled(false);
            analysisAction->setEnabled(false);
            break;
            
        case GameState::MATCHMAKING:
            centralStack->setCurrentWidget(mainMenuWidget);
            matchmakingAction->setEnabled(false);
            break;
            
        case GameState::PLAYING:
            centralStack->setCurrentWidget(gameWidget);
            matchmakingAction->setEnabled(false);
            drawAction->setEnabled(true);
            resignAction->setEnabled(true);
            analysisAction->setEnabled(false);
            break;
            
        case GameState::GAME_OVER:
            centralStack->setCurrentWidget(gameWidget);
            matchmakingAction->setEnabled(true);
            drawAction->setEnabled(false);
            resignAction->setEnabled(false);
            analysisAction->setEnabled(true);
            break;
    }
}

void ChessClient::connectToServer() {
    // Ask for server details
    bool ok;
    QString hostPort = QInputDialog::getText(
        this, "Connect to Server", "Enter server address (host:port):",
        QLineEdit::Normal, serverHost + ":" + QString::number(serverPort), &ok);
    
    if (!ok || hostPort.isEmpty()) {
        return;
    }
    
    // Parse host and port
    QStringList parts = hostPort.split(":");
    QString host = parts[0];
    quint16 port = (parts.size() > 1) ? static_cast<quint16>(parts[1].toInt()) : 8080;
    
    // Save settings
    serverHost = host;
    serverPort = port;
    
    // Connect to server
    statusLabel->setText("Connecting to " + host + ":" + QString::number(port) + "...");
    if (!NetworkManager::getInstance().connectToServer(host, port)) {
        statusLabel->setText("Failed to connect to server");
        QMessageBox::critical(this, "Connection Error", "Failed to connect to server");
    }
}

void ChessClient::disconnectFromServer() {
    NetworkManager::getInstance().disconnectFromServer();
}

void ChessClient::handleNetworkMessage(const Message& message) {
    switch (message.type) {
        case MessageType::LOGIN:
            handleLoginResult(message.payload);
            break;
        case MessageType::REGISTER:
            handleRegistrationResult(message.payload);
            break;
        case MessageType::MATCHMAKING_STATUS:
            handleMatchmakingStatus(message.payload);
            break;
        case MessageType::GAME_START:
            handleGameStart(message.payload);
            break;
        case MessageType::MOVE_RESULT:
            handleMoveResult(message.payload);
            break;
        case MessageType::POSSIBLE_MOVES:
            handlePossibleMoves(message.payload);
            break;
        case MessageType::GAME_END:
            handleGameEnd(message.payload);
            break;
        case MessageType::TIME_UPDATE:
            handleTimeUpdate(message.payload);
            break;
        case MessageType::GAME_ANALYSIS:
            handleGameAnalysis(message.payload);
            break;
        case MessageType::PLAYER_STATS:
            handlePlayerStats(message.payload);
            break;
        case MessageType::LEADERBOARD_RESPONSE:
            handleLeaderboard(message.payload);
            break;
        case MessageType::MOVE_RECOMMENDATIONS:
            handleMoveRecommendations(message.payload);
            break;
        case MessageType::ERROR:
            QMessageBox::warning(this, "Server Error", QString::fromUtf8(message.payload));
            break;
        default:
            qDebug() << "Unhandled message type:" << static_cast<int>(message.type);
            break;
    }
}

void ChessClient::handleLoginResult(const QByteArray& payload) {
    QString status = parsePayloadValue(payload, "STATUS");
    
    if (status == "SUCCESS") {
        QString username = parsePayloadValue(payload, "USERNAME");
        int rating = parsePayloadValue(payload, "RATING").toInt();
        
        loggedIn(username, rating);
        setState(GameState::MAIN_MENU);
        QMessageBox::information(this, "Login Successful", "Welcome, " + username + "!");
    } else {
        QString message = parsePayloadValue(payload, "MESSAGE");
        QMessageBox::warning(this, "Login Failed", message.isEmpty() ? "Invalid username or password" : message);
    }
}

void ChessClient::handleRegistrationResult(const QByteArray& payload) {
    QString status = parsePayloadValue(payload, "STATUS");
    
    if (status == "SUCCESS") {
        QString username = parsePayloadValue(payload, "USERNAME");
        QMessageBox::information(this, "Registration Successful", 
                               "Account created successfully!\nYou can now login as " + username);
    } else {
        QString message = parsePayloadValue(payload, "MESSAGE");
        QMessageBox::warning(this, "Registration Failed", 
                           message.isEmpty() ? "Failed to create account" : message);
    }
}

void ChessClient::handleMatchmakingStatus(const QByteArray& payload) {
    QString status = parsePayloadValue(payload, "STATUS");
    
    if (status == "SEARCHING") {
        QString rating = parsePayloadValue(payload, "RATING");
        matchmakingDialog->updateStatus("Searching for opponent with similar rating (" + rating + ")...");
    }
    else if (status == "MATCHED") {
        currentGameId = parsePayloadValue(payload, "GAME_ID").toUInt();
        QString colorStr = parsePayloadValue(payload, "COLOR");
        isWhitePlayer = (colorStr == "WHITE");
        opponentName = parsePayloadValue(payload, "OPPONENT");
        opponentRating = parsePayloadValue(payload, "OPPONENT_RATING").toInt();
        isOpponentBot = false;
        
        matchmakingDialog->updateStatus("Opponent found! Starting game...");
        QTimer::singleShot(1000, matchmakingDialog, &QDialog::accept);
        
        // Game will start when GAME_START message is received
    }
    else if (status == "MATCHED_BOT") {
        currentGameId = parsePayloadValue(payload, "GAME_ID").toUInt();
        QString colorStr = parsePayloadValue(payload, "COLOR");
        isWhitePlayer = (colorStr == "WHITE");
        opponentName = "ChessBot";
        isOpponentBot = true;
        
        int botDifficulty = parsePayloadValue(payload, "BOT_DIFFICULTY").toInt();
        // Map bot difficulty to approximate rating
        switch (botDifficulty) {
            case 1: opponentRating = 800; break;
            case 2: opponentRating = 1000; break;
            case 3: opponentRating = 1400; break;
            case 4: opponentRating = 1700; break;
            case 5: opponentRating = 2000; break;
            default: opponentRating = 1200; break;
        }
        
        matchmakingDialog->updateStatus("Matched with Bot (Level " + QString::number(botDifficulty) + ")");
        QTimer::singleShot(1000, matchmakingDialog, &QDialog::accept);
        
        // Game will start when GAME_START message is received
    }
    else if (status == "CANCELLED") {
        matchmakingDialog->updateStatus("Matchmaking cancelled");
        QTimer::singleShot(1000, matchmakingDialog, &QDialog::accept);
    }
}

void ChessClient::handleGameStart(const QByteArray& payload) {
    // Parse player information
    QStringList parts = QString::fromUtf8(payload).split(";");
    
    for (const QString& part : parts) {
        if (part.startsWith("WHITE:")) {
            QString whiteName = part.mid(6);
            if (whiteName == username) {
                isWhitePlayer = true;
            }
        }
        else if (part.startsWith("BLACK:")) {
            QString blackName = part.mid(6);
            if (blackName == username) {
                isWhitePlayer = false;
            }
        }
        else if (part.startsWith("TIME_CONTROL:")) {
            QStringList timeInfo = part.mid(13).split(",");
            if (timeInfo.size() >= 2) {
                int initialTime = timeInfo[0].toInt();
                whiteRemainingTime = blackRemainingTime = initialTime;
            }
        }
    }
    
    // Set up the game UI
    chessBoard->resetBoard();
    chessBoard->setRotated(!isWhitePlayer);
    
    // Update player info
    playerInfoWidget->setPlayerInfo(username, userRating, 
                                  opponentName, opponentRating,
                                  isWhitePlayer);
    
    // Clear analysis and recommendations
    analysisWidget->clear();
    recommendedMoves.clear();
    
    // Update status
    statusLabel->setText("Game started - " + (isWhitePlayer ? "Playing as White" : "Playing as Black"));
    
    // Show game screen
    setState(GameState::PLAYING);
}

void ChessClient::handleMoveResult(const QByteArray& payload) {
    QString fen = parsePayloadValue(payload, "FEN");
    QString lastMove = parsePayloadValue(payload, "LAST_MOVE");
    QString notation = parsePayloadValue(payload, "NOTATION");
    QString isCheck = parsePayloadValue(payload, "CHECK");
    QString isCheckmate = parsePayloadValue(payload, "CHECKMATE");
    QString isStalemate = parsePayloadValue(payload, "STALEMATE");
    
    // Update chess board
    chessBoard->setPosition(fen);
    
    // Play sound
    if (!lastMove.isEmpty()) {
        if (isCheck == "1") {
            checkSound->play();
        } else if (notation.contains("x")) {
            captureSound->play();
        } else {
            moveSound->play();
        }
    }
    
    // Update status message
    if (isCheckmate == "1") {
        statusLabel->setText("Checkmate!");
        gameEndSound->play();
    } else if (isStalemate == "1") {
        statusLabel->setText("Stalemate!");
        gameEndSound->play();
    } else if (isCheck == "1") {
        statusLabel->setText("Check!");
    } else {
        statusLabel->setText("Your move");
    }
}

void ChessClient::handlePossibleMoves(const QByteArray& payload) {
    QString movesStr = parsePayloadValue(payload, "MOVES");
    QStringList movesList = movesStr.split(",", Qt::SkipEmptyParts);
    
    possibleMoves.clear();
    possibleMoves = QVector<QString>::fromList(movesList);
    
    chessBoard->setPossibleMoves(movesList);
}

void ChessClient::handleGameEnd(const QByteArray& payload) {
    QString result = parsePayloadValue(payload, "RESULT");
    
    QString message;
    if (result == "WHITE_WON_CHECKMATE") {
        message = "White wins by checkmate!";
    } else if (result == "BLACK_WON_CHECKMATE") {
        message = "Black wins by checkmate!";
    } else if (result == "WHITE_WON_TIME") {
        message = "White wins on time!";
    } else if (result == "BLACK_WON_TIME") {
        message = "Black wins on time!";
    } else if (result == "WHITE_WON_RESIGNATION") {
        message = "White wins - Black resigned!";
    } else if (result == "BLACK_WON_RESIGNATION") {
        message = "Black wins - White resigned!";
    } else if (result == "DRAW_AGREEMENT") {
        message = "Game drawn by agreement!";
    } else if (result == "DRAW_STALEMATE") {
        message = "Game drawn by stalemate!";
    } else if (result == "DRAW_REPETITION") {
        message = "Game drawn by threefold repetition!";
    } else if (result == "DRAW_FIFTY_MOVE") {
        message = "Game drawn by fifty-move rule!";
    } else if (result == "DRAW_INSUFFICIENT") {
        message = "Game drawn by insufficient material!";
    } else if (result == "OPPONENT_DISCONNECTED") {
        message = "Your opponent disconnected. You win!";
    } else {
        message = "Game over: " + result;
    }
    
    // Play sound
    gameEndSound->play();
    
    // Update UI
    setState(GameState::GAME_OVER);
    statusLabel->setText(message);
    
    // Show dialog
    QMessageBox::information(this, "Game Over", message);
    
    // Request game analysis
    requestGameAnalysis();
}

void ChessClient::handleTimeUpdate(const QByteArray& payload) {
    whiteRemainingTime = parsePayloadValue(payload, "WHITE").toInt();
    blackRemainingTime = parsePayloadValue(payload, "BLACK").toInt();
    
    // Update UI
    playerInfoWidget->updateTime(whiteRemainingTime, blackRemainingTime);
}

void ChessClient::handleGameAnalysis(const QByteArray& payload) {
    int whiteAccuracy = parsePayloadValue(payload, "WHITE_ACCURACY").toInt();
    int blackAccuracy = parsePayloadValue(payload, "BLACK_ACCURACY").toInt();
    
    // Parse annotations
    QVector<QString> annotations;
    int annotationCount = parsePayloadValue(payload, "ANNOTATIONS").toInt();
    
    for (int i = 0; i < annotationCount; i++) {
        QString annotation = parsePayloadValue(payload, "ANN" + QString::number(i));
        if (!annotation.isEmpty()) {
            annotations.append(annotation);
        }
    }
    
    // Parse evaluations
    QVector<int> evaluations;
    int evalCount = parsePayloadValue(payload, "EVALUATIONS").toInt();
    
    for (int i = 0; i < evalCount; i++) {
        QString evalKey = "EVAL" + QString::number(i);
        if (payload.contains(evalKey.toUtf8())) {
            int eval = parsePayloadValue(payload, evalKey).toInt();
            evaluations.append(eval);
        }
    }
    
    // Update analysis widget
    analysisWidget->setAnalysisData(whiteAccuracy, blackAccuracy, annotations, evaluations);
}

void ChessClient::handlePlayerStats(const QByteArray& payload) {
    QString username = parsePayloadValue(payload, "USERNAME");
    int rating = parsePayloadValue(payload, "RATING").toInt();
    int gamesPlayed = parsePayloadValue(payload, "GAMES_PLAYED").toInt();
    int wins = parsePayloadValue(payload, "WINS").toInt();
    int losses = parsePayloadValue(payload, "LOSSES").toInt();
    int draws = parsePayloadValue(payload, "DRAWS").toInt();
    double winPercentage = parsePayloadValue(payload, "WIN_PERCENTAGE").toDouble();
    
    // Show stats dialog
    QMessageBox statsDialog(this);
    statsDialog.setWindowTitle("Player Statistics: " + username);
    statsDialog.setIcon(QMessageBox::Information);
    
    QString statsText = QString(
        "<h2>%1</h2>"
        "<p>Rating: <b>%2</b></p>"
        "<p>Games Played: %3</p>"
        "<p>Wins: %4</p>"
        "<p>Losses: %5</p>"
        "<p>Draws: %6</p>"
        "<p>Win Percentage: %7%</p>"
    ).arg(username)
     .arg(rating)
     .arg(gamesPlayed)
     .arg(wins)
     .arg(losses)
     .arg(draws)
     .arg(winPercentage, 0, 'f', 1);
    
    statsDialog.setText(statsText);
    statsDialog.exec();
}

void ChessClient::handleLeaderboard(const QByteArray& payload) {
    int count = parsePayloadValue(payload, "COUNT").toInt();
    QVector<QPair<QString, int>> leaderboardData;
    
    // Parse player data
    for (int i = 0; i < count; i++) {
        QString playerKey = "PLAYER" + QString::number(i);
        QString playerData = parsePayloadValue(payload, playerKey);
        
        if (!playerData.isEmpty()) {
            QStringList parts = playerData.split(",");
            if (parts.size() >= 2) {
                QString name = parts[0];
                int rating = parts[1].toInt();
                leaderboardData.append(qMakePair(name, rating));
            }
        }
    }
    
    // Update leaderboard dialog
    leaderboardDialog->updateLeaderboard(leaderboardData);
    leaderboardDialog->exec();
}

void ChessClient::handleMoveRecommendations(const QByteArray& payload) {
    int count = parsePayloadValue(payload, "COUNT").toInt();
    recommendedMoves.clear();
    
    // Parse recommendations
    for (int i = 0; i < count; i++) {
        QString moveKey = "MOVE" + QString::number(i);
        QString moveData = parsePayloadValue(payload, moveKey);
        
        if (!moveData.isEmpty()) {
            QStringList parts = moveData.split(",");
            if (parts.size() >= 2) {
                QString moveStr = parts[0];
                double probability = parts[1].toDouble();
                recommendedMoves.append(qMakePair(moveStr, probability));
            }
        }
    }
    
    // Highlight recommended moves on the board
    QStringList highlightMoves;
    for (const auto& recommendation : recommendedMoves) {
        highlightMoves << recommendation.first;
    }
    chessBoard->setRecommendedMoves(highlightMoves);
    
    // Show recommendations in a dialog
    QDialog recDialog(this);
    recDialog.setWindowTitle("Move Recommendations");
    QVBoxLayout* recLayout = new QVBoxLayout(&recDialog);
    
    QLabel* titleLabel = new QLabel("Recommended Moves:");
    titleLabel->setStyleSheet("font-weight: bold; font-size: 14px;");
    recLayout->addWidget(titleLabel);
    
    for (const auto& rec : recommendedMoves) {
        QString moveStr = rec.first;
        double probability = rec.second;
        
        // Create a nicer display for the move
        QString from = moveStr.left(2);
        QString to = moveStr.mid(2, 2);
        
        QWidget* moveWidget = new QWidget;
        QHBoxLayout* moveLayout = new QHBoxLayout(moveWidget);
        
        QLabel* moveLabel = new QLabel(from + " → " + to);
        QLabel* probLabel = new QLabel(QString::number(probability, 'f', 1) + "%");
        
        QPushButton* playBtn = new QPushButton("Play");
        connect(playBtn, &QPushButton::clicked, this, [this, from, to, &recDialog]() {
            makeBoardMove(from, to);
            recDialog.accept();
        });
        
        moveLayout->addWidget(moveLabel);
        moveLayout->addWidget(probLabel);
        moveLayout->addWidget(playBtn);
        
        recLayout->addWidget(moveWidget);
    }
    
    QPushButton* closeBtn = new QPushButton("Close");
    connect(closeBtn, &QPushButton::clicked, &recDialog, &QDialog::accept);
    recLayout->addWidget(closeBtn);
    
    recDialog.setLayout(recLayout);
    recDialog.exec();
}

void ChessClient::showLogin() {
    if (!NetworkManager::getInstance().isConnected()) {
        connectToServer();
    }
    
    if (NetworkManager::getInstance().isConnected()) {
        loginDialog->exec();
        
        if (loginDialog->result() == QDialog::Accepted) {
            QString username = loginDialog->getUsername();
            QString password = loginDialog->getPassword();
            
            // Send login message
            Message loginMsg;
            loginMsg.type = MessageType::LOGIN;
            loginMsg.payload = QString("USERNAME:%1;PASSWORD:%2;").arg(username).arg(password).toUtf8();
            
            NetworkManager::getInstance().sendMessage(loginMsg);
        }
    }
}

void ChessClient::showRegistration() {
    if (!NetworkManager::getInstance().isConnected()) {
        connectToServer();
    }
    
    if (NetworkManager::getInstance().isConnected()) {
        registrationDialog->exec();
        
        if (registrationDialog->result() == QDialog::Accepted) {
            QString username = registrationDialog->getUsername();
            QString password = registrationDialog->getPassword();
            
            // Send registration message
            Message regMsg;
            regMsg.type = MessageType::REGISTER;
            regMsg.payload = QString("USERNAME:%1;PASSWORD:%2;").arg(username).arg(password).toUtf8();
            
            NetworkManager::getInstance().sendMessage(regMsg);
        }
    }
}

void ChessClient::showMainMenu() {
    setState(GameState::MAIN_MENU);
}

void ChessClient::showMatchmaking() {
    if (!NetworkManager::getInstance().isConnected() || username.isEmpty()) {
        QMessageBox::warning(this, "Not Logged In", "You must be logged in to find a game");
        return;
    }
    
    matchmakingDialog->reset();
    matchmakingDialog->show();
    
    // Send matchmaking request
    Message request;
    request.type = MessageType::MATCHMAKING_REQUEST;
    
    // User can select time control in the matchmaking dialog
    QString timeControl = matchmakingDialog->getSelectedTimeControl();
    
    request.payload = QString("USERNAME:%1;TIME_CONTROL:%2;CANCEL:0;")
                       .arg(username)
                       .arg(timeControl)
                       .toUtf8();
    
    NetworkManager::getInstance().sendMessage(request);
    
    setState(GameState::MATCHMAKING);
}

void ChessClient::showLeaderboard() {
    if (!NetworkManager::getInstance().isConnected()) {
        QMessageBox::warning(this, "Not Connected", "You must be connected to view the leaderboard");
        return;
    }
    
    // Request leaderboard data
    Message request;
    request.type = MessageType::LEADERBOARD_REQUEST;
    request.payload = "COUNT:20;"; // Request top 20 players
    
    NetworkManager::getInstance().sendMessage(request);
}

void ChessClient::showPlayerStats() {
    if (!NetworkManager::getInstance().isConnected()) {
        QMessageBox::warning(this, "Not Connected", "You must be connected to view player statistics");
        return;
    }
    
    // Ask for username
    bool ok;
    QString playerName = QInputDialog::getText(
        this, "Player Statistics", "Enter username:",
        QLineEdit::Normal, username, &ok);
    
    if (!ok || playerName.isEmpty()) {
        return;
    }
    
    // Request player statistics
    Message request;
    request.type = MessageType::PLAYER_STATS;
    request.payload = "USERNAME:" + playerName.toUtf8() + ";";
    
    NetworkManager::getInstance().sendMessage(request);
}

void ChessClient::updateTimers() {
    // This method is called by the timer to update time displays
    if (currentState == GameState::PLAYING) {
        playerInfoWidget->updateTime(whiteRemainingTime, blackRemainingTime);
    }
}

void ChessClient::makeBoardMove(const QString& from, const QString& to) {
    // Check if it's a valid move
    QString moveStr = from + to;
    if (!possibleMoves.contains(moveStr)) {
        QMessageBox::warning(this, "Invalid Move", "That move is not allowed");
        return;
    }
    
    // Send move to server
    Message moveMsg;
    moveMsg.type = MessageType::MOVE;
    moveMsg.payload = QString("MOVE:%1;").arg(moveStr).toUtf8();
    
    NetworkManager::getInstance().sendMessage(moveMsg);
    
    // Clear recommended moves highlight
    chessBoard->clearRecommendedMoves();
}

void ChessClient::requestDraw() {
    if (currentState != GameState::PLAYING) {
        return;
    }
    
    // Confirm with user
    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "Offer Draw", "Are you sure you want to offer a draw?",
        QMessageBox::Yes | QMessageBox::No);
        
    if (reply != QMessageBox::Yes) {
        return;
    }
    
    // Send draw request
    Message drawMsg;
    drawMsg.type = MessageType::REQUEST_DRAW;
    
    NetworkManager::getInstance().sendMessage(drawMsg);
    statusLabel->setText("Draw offered. Waiting for opponent's response...");
}

void ChessClient::resignGame() {
    if (currentState != GameState::PLAYING) {
        return;
    }
    
    // Confirm with user
    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "Resign Game", "Are you sure you want to resign?",
        QMessageBox::Yes | QMessageBox::No);
        
    if (reply != QMessageBox::Yes) {
        return;
    }
    
    // Send resignation
    Message resignMsg;
    resignMsg.type = MessageType::RESIGN;
    
    NetworkManager::getInstance().sendMessage(resignMsg);
}

void ChessClient::requestMoveRecommendations() {
    if (currentState != GameState::PLAYING) {
        return;
    }
    
    // Send recommendation request
    Message recMsg;
    recMsg.type = MessageType::MOVE_RECOMMENDATIONS;
    recMsg.payload = QString("GAME_ID:%1;MAX_MOVES:5;").arg(currentGameId).toUtf8();
    
    NetworkManager::getInstance().sendMessage(recMsg);
    statusLabel->setText("Requesting move recommendations...");
}

void ChessClient::requestGameAnalysis() {
    if (currentState != GameState::GAME_OVER && currentState != GameState::PLAYING) {
        return;
    }
    
    // Send analysis request
    Message analysisMsg;
    analysisMsg.type = MessageType::GAME_ANALYSIS;
    analysisMsg.payload = QString("GAME_ID:%1;").arg(currentGameId).toUtf8();
    
    NetworkManager::getInstance().sendMessage(analysisMsg);
    statusLabel->setText("Analyzing game...");
}

void ChessClient::loggedIn(const QString& username, int rating) {
    this->username = username;
    this->userRating = rating;
    
    // Update window title
    setWindowTitle("Chess Client - " + username + " (" + QString::number(rating) + ")");
    
    // Update status bar
    statusLabel->setText("Logged in as " + username);
}

void ChessClient::saveSettings() {
    QSettings settings("ChessClient", "Settings");
    
    settings.setValue("ServerHost", serverHost);
    settings.setValue("ServerPort", serverPort);
    settings.setValue("Username", username);
}

void ChessClient::loadSettings() {
    QSettings settings("ChessClient", "Settings");
    
    serverHost = settings.value("ServerHost", "localhost").toString();
    serverPort = settings.value("ServerPort", 8080).toUInt();
    // Don't load username - require login each time for security
}

QString ChessClient::parsePayloadValue(const QByteArray& payload, const QString& key) {
    QString payloadStr = QString::fromUtf8(payload);
    int startIndex = payloadStr.indexOf(key + ":");
    
    if (startIndex == -1) {
        return QString();
    }
    
    startIndex += key.length() + 1; // Skip "KEY:"
    
    int endIndex = payloadStr.indexOf(";", startIndex);
    if (endIndex == -1) {
        return payloadStr.mid(startIndex);
    }
    
    return payloadStr.mid(startIndex, endIndex - startIndex);
}

QMap<QString, QString> ChessClient::parsePayloadMap(const QByteArray& payload) {
    QString payloadStr = QString::fromUtf8(payload);
    QStringList parts = payloadStr.split(";", Qt::SkipEmptyParts);
    
    QMap<QString, QString> result;
    
    for (const QString& part : parts) {
        int colonIndex = part.indexOf(":");
        if (colonIndex != -1) {
            QString key = part.left(colonIndex);
            QString value = part.mid(colonIndex + 1);
            result[key] = value;
        }
    }
    
    return result;
}