// ChessClient.h
#pragma once

#include <QMainWindow>
#include <QStackedWidget>
#include <QToolBar>
#include <QStatusBar>
#include <QLabel>
#include <QTimer>
#include <QAction>
#include <QMenu>
#include <QMap>
#include <QString>
#include <QVector>
#include <QPair>
#include <QSound>

#include "NetworkManager.h"
#include "ChessBoard.h"
#include "PlayerInfoWidget.h"
#include "GameAnalysisWidget.h"

class LoginDialog;
class RegistrationDialog;
class MatchmakingDialog;
class LeaderboardDialog;

enum class GameState {
    LOGIN,
    MAIN_MENU,
    MATCHMAKING,
    PLAYING,
    GAME_OVER
};

class ChessClient : public QMainWindow {
    Q_OBJECT

public:
    explicit ChessClient(QWidget* parent = nullptr);
    ~ChessClient();

private slots:
    void connectToServer();
    void disconnectFromServer();
    
    void handleNetworkMessage(const Message& message);
    
    void handleLoginResult(const QByteArray& payload);
    void handleRegistrationResult(const QByteArray& payload);
    void handleMatchmakingStatus(const QByteArray& payload);
    void handleGameStart(const QByteArray& payload);
    void handleMoveResult(const QByteArray& payload);
    void handlePossibleMoves(const QByteArray& payload);
    void handleGameEnd(const QByteArray& payload);
    void handleTimeUpdate(const QByteArray& payload);
    void handleGameAnalysis(const QByteArray& payload);
    void handlePlayerStats(const QByteArray& payload);
    void handleLeaderboard(const QByteArray& payload);
    void handleMoveRecommendations(const QByteArray& payload);
    
    void showLogin();
    void showRegistration();
    void showMainMenu();
    void showMatchmaking();
    void showLeaderboard();
    void showPlayerStats();
    
    void updateTimers();
    void makeBoardMove(const QString& from, const QString& to);
    void requestDraw();
    void resignGame();
    void requestMoveRecommendations();
    void requestGameAnalysis();
    
    void loggedIn(const QString& username, int rating);
    
private:
    void createActions();
    void createMenus();
    void createToolbars();
    void setupUI();
    void setupConnections();
    void setState(GameState state);
    
    void saveSettings();
    void loadSettings();
    
    // Helper methods
    QString parsePayloadValue(const QByteArray& payload, const QString& key);
    QMap<QString, QString> parsePayloadMap(const QByteArray& payload);
    
    // Network & authentication
    QString serverHost;
    quint16 serverPort;
    QString username;
    int userRating;
    
    // Game data
    quint32 currentGameId;
    bool isWhitePlayer;
    QString opponentName;
    int opponentRating;
    bool isOpponentBot;
    
    int whiteRemainingTime;
    int blackRemainingTime;
    QVector<QString> possibleMoves;
    
    QVector<QPair<QString, double>> recommendedMoves;
    
    // UI components
    GameState currentState;
    QStackedWidget* centralStack;
    
    QWidget* mainMenuWidget;
    QWidget* gameWidget;
    
    ChessBoard* chessBoard;
    PlayerInfoWidget* playerInfoWidget;
    GameAnalysisWidget* analysisWidget;
    
    LoginDialog* loginDialog;
    RegistrationDialog* registrationDialog;
    MatchmakingDialog* matchmakingDialog;
    LeaderboardDialog* leaderboardDialog;
    
    QLabel* statusLabel;
    QLabel* connectionLabel;
    QTimer* gameTimer;
    
    // Sounds
    QSound* moveSound;
    QSound* captureSound;
    QSound* checkSound;
    QSound* gameEndSound;
    
    // Actions
    QAction* connectAction;
    QAction* disconnectAction;
    QAction* loginAction;
    QAction* registerAction;
    QAction* matchmakingAction;
    QAction* leaderboardAction;
    QAction* playerStatsAction;
    QAction* drawAction;
    QAction* resignAction;
    QAction* analysisAction;
};