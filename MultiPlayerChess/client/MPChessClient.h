// MPChessClient.h

#ifndef MPCHESSCLIENT_H
#define MPCHESSCLIENT_H

#include <QMainWindow>
#include <QTcpSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTimer>
#include <QSettings>
#include <QStackedWidget>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QListWidget>
#include <QTableWidget>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QSvgRenderer>
#include <QSound>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QSlider>
#include <QSpinBox>
#include <QTabWidget>
#include <QSplitter>
#include <QScrollArea>
#include <QMessageBox>
#include <QInputDialog>
#include <QFileDialog>
#include <QProgressBar>
#include <QStatusBar>
#include <QMenuBar>
#include <QToolBar>
#include <QAction>
#include <QSystemTrayIcon>
#include <QCloseEvent>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QFuture>
#include <QtConcurrent>
#include <QChart>
#include <QChartView>
#include <QLineSeries>
#include <QValueAxis>
#include <QBarSeries>
#include <QBarSet>
#include <QPieSeries>
#include <QPieSlice>
#include <QGraphicsDropShadowEffect>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>
#include <QSequentialAnimationGroup>
#include <QStateMachine>
#include <QState>
#include <QSignalTransition>
#include <QEventTransition>
#include <QLoggingCategory>

#include <vector>
#include <map>
#include <unordered_map>
#include <memory>
#include <functional>
#include <optional>
#include <chrono>
#include <random>
#include <algorithm>
#include <utility>
#include <cmath>
#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>

QT_BEGIN_NAMESPACE
namespace Ui { class MPChessClient; }
QT_END_NAMESPACE

// Forward declarations
class ChessBoardWidget;
class ChessPieceItem;
class NetworkManager;
class GameManager;
class ThemeManager;
class AudioManager;
class AnalysisWidget;
class MoveHistoryWidget;
class CapturedPiecesWidget;
class GameTimerWidget;
class ProfileWidget;
class LeaderboardWidget;
class MatchmakingWidget;
class GameHistoryWidget;
class SettingsDialog;
class LoginDialog;
class RegisterDialog;
class PromotionDialog;
class Logger;

/**
 * @brief Enum representing the type of chess piece
 */
enum class PieceType {
    PAWN,
    KNIGHT,
    BISHOP,
    ROOK,
    QUEEN,
    KING,
    EMPTY
};

/**
 * @brief Enum representing the color of a chess piece
 */
enum class PieceColor {
    WHITE,
    BLACK,
    NONE
};

/**
 * @brief Enum representing the result of a chess game
 */
enum class GameResult {
    WHITE_WIN,
    BLACK_WIN,
    DRAW,
    IN_PROGRESS
};

/**
 * @brief Enum representing the game time control type
 */
enum class TimeControlType {
    RAPID,      // 10 minutes per player
    BLITZ,      // 5 minutes per player
    BULLET,     // 1 minute per player
    CLASSICAL,  // 90 minutes per player
    CASUAL      // 7 days per move
};

/**
 * @brief Enum representing the type of message sent between client and server
 */
enum class MessageType {
    AUTHENTICATION,
    AUTHENTICATION_RESULT,
    GAME_START,
    MOVE,
    MOVE_RESULT,
    GAME_STATE,
    GAME_OVER,
    CHAT,
    MOVE_RECOMMENDATIONS,
    MATCHMAKING_REQUEST,
    MATCHMAKING_STATUS,
    ERROR,
    PING,
    PONG,
    GAME_HISTORY_REQUEST,
    GAME_HISTORY_RESPONSE,
    GAME_ANALYSIS_REQUEST,
    GAME_ANALYSIS_RESPONSE,
    RESIGN,
    DRAW_OFFER,
    DRAW_RESPONSE,
    LEADERBOARD_REQUEST,
    LEADERBOARD_RESPONSE
};

/**
 * @brief Struct representing a position on the chess board
 */
struct Position {
    int row;    // 0-7, 0 is the white's back rank
    int col;    // 0-7, 0 is the a-file
    
    Position() : row(-1), col(-1) {}
    Position(int r, int c) : row(r), col(c) {}
    
    bool isValid() const {
        return row >= 0 && row < 8 && col >= 0 && col < 8;
    }
    
    bool operator==(const Position& other) const {
        return row == other.row && col == other.col;
    }
    
    bool operator!=(const Position& other) const {
        return !(*this == other);
    }
    
    // Convert to algebraic notation (e.g., "e4")
    QString toAlgebraic() const {
        if (!isValid()) return "invalid";
        return QString(QChar('a' + col)) + QString::number(row + 1);
    }
    
    // Create from algebraic notation (e.g., "e4")
    static Position fromAlgebraic(const QString& algebraic) {
        if (algebraic.length() != 2) return Position();
        int col = algebraic[0].toLatin1() - 'a';
        int row = algebraic[1].digitValue() - 1;
        if (col < 0 || col > 7 || row < 0 || row > 7) return Position();
        return Position(row, col);
    }
};

/**
 * @brief Class representing a chess move
 */
class ChessMove {
public:
    ChessMove() : from(-1, -1), to(-1, -1), promotionType(PieceType::EMPTY) {}
    
    ChessMove(const Position& from, const Position& to, PieceType promotionType = PieceType::EMPTY)
        : from(from), to(to), promotionType(promotionType) {}
    
    Position getFrom() const { return from; }
    Position getTo() const { return to; }
    PieceType getPromotionType() const { return promotionType; }
    void setPromotionType(PieceType type) { promotionType = type; }
    
    // Convert to algebraic notation (e.g., "e2e4" or "e7e8q" for promotion)
    QString toAlgebraic() const {
        QString result = from.toAlgebraic() + to.toAlgebraic();
        
        if (promotionType != PieceType::EMPTY) {
            QChar promotionChar;
            switch (promotionType) {
                case PieceType::QUEEN:  promotionChar = 'q'; break;
                case PieceType::ROOK:   promotionChar = 'r'; break;
                case PieceType::BISHOP: promotionChar = 'b'; break;
                case PieceType::KNIGHT: promotionChar = 'n'; break;
                default:                promotionChar = 'q'; break;
            }
            result += promotionChar;
        }
        
        return result;
    }
    
    // Create from algebraic notation
    static ChessMove fromAlgebraic(const QString& algebraic) {
        if (algebraic.length() < 4) return ChessMove();
        
        Position from = Position::fromAlgebraic(algebraic.left(2));
        Position to = Position::fromAlgebraic(algebraic.mid(2, 2));
        
        PieceType promotionType = PieceType::EMPTY;
        if (algebraic.length() > 4) {
            QChar promotionChar = algebraic[4];
            switch (promotionChar.toLatin1()) {
                case 'q': promotionType = PieceType::QUEEN; break;
                case 'r': promotionType = PieceType::ROOK; break;
                case 'b': promotionType = PieceType::BISHOP; break;
                case 'n': promotionType = PieceType::KNIGHT; break;
                default:  promotionType = PieceType::QUEEN; break;
            }
        }
        
        return ChessMove(from, to, promotionType);
    }
    
    bool operator==(const ChessMove& other) const {
        return from == other.from && to == other.to && promotionType == other.promotionType;
    }
    
    bool operator!=(const ChessMove& other) const {
        return !(*this == other);
    }

private:
    Position from;
    Position to;
    PieceType promotionType;
};

/**
 * @brief Class representing a chess piece
 */
class ChessPiece {
public:
    ChessPiece(PieceType type, PieceColor color) : type(type), color(color) {}
    virtual ~ChessPiece() = default;
    
    PieceType getType() const { return type; }
    PieceColor getColor() const { return color; }
    
    // Get the character representation of this piece for ASCII display
    QChar getAsciiChar() const {
        QChar c;
        switch (type) {
            case PieceType::PAWN:   c = 'p'; break;
            case PieceType::KNIGHT: c = 'n'; break;
            case PieceType::BISHOP: c = 'b'; break;
            case PieceType::ROOK:   c = 'r'; break;
            case PieceType::QUEEN:  c = 'q'; break;
            case PieceType::KING:   c = 'k'; break;
            default:                c = ' '; break;
        }
        
        if (color == PieceColor::WHITE) {
            c = c.toUpper();
        }
        
        return c;
    }
    
    // Get the SVG file name for this piece based on the current theme
    QString getSvgFileName(const QString& theme) const {
        QString colorStr = (color == PieceColor::WHITE) ? "white" : "black";
        QString typeStr;
        
        switch (type) {
            case PieceType::PAWN:   typeStr = "pawn"; break;
            case PieceType::KNIGHT: typeStr = "knight"; break;
            case PieceType::BISHOP: typeStr = "bishop"; break;
            case PieceType::ROOK:   typeStr = "rook"; break;
            case PieceType::QUEEN:  typeStr = "queen"; break;
            case PieceType::KING:   typeStr = "king"; break;
            default:                typeStr = ""; break;
        }
        
        return QString(":/pieces/%1/%2_%3.svg").arg(theme, colorStr, typeStr);
    }

private:
    PieceType type;
    PieceColor color;
};

/**
 * @brief Class for logging client events
 */
class Logger : public QObject {
    Q_OBJECT
    
public:
    enum class LogLevel {
        DEBUG,
        INFO,
        WARNING,
        ERROR
    };
    
    Logger(QObject* parent = nullptr);
    ~Logger();
    
    void setLogLevel(LogLevel level);
    LogLevel getLogLevel() const;
    
    void debug(const QString& message);
    void info(const QString& message);
    void warning(const QString& message);
    void error(const QString& message);
    
    void setLogToFile(bool enabled, const QString& filePath = QString());
    bool isLoggingToFile() const;
    
    QString getLogFilePath() const;

signals:
    void logMessage(Logger::LogLevel level, const QString& message);

private:
    LogLevel logLevel;
    bool logToFile;
    QString logFilePath;
    QFile logFile;
    QMutex mutex;
    
    void log(LogLevel level, const QString& message);
    QString levelToString(LogLevel level) const;
    QString getCurrentTimestamp() const;
};

/**
 * @brief Class for managing network communication with the server
 */
class NetworkManager : public QObject {
    Q_OBJECT
    
public:
    NetworkManager(Logger* logger, QObject* parent = nullptr);
    ~NetworkManager();
    
    bool connectToServer(const QString& host, int port);
    void disconnectFromServer();
    bool isConnected() const;
    
    void authenticate(const QString& username, const QString& password, bool isRegistration = false);
    void sendMove(const QString& gameId, const ChessMove& move);
    void requestMatchmaking(bool join, TimeControlType timeControl = TimeControlType::RAPID);
    void requestGameHistory();
    void requestGameAnalysis(const QString& gameId);
    void sendResignation(const QString& gameId);
    void sendDrawOffer(const QString& gameId);
    void sendDrawResponse(const QString& gameId, bool accepted);
    void requestLeaderboard(bool allPlayers = false, int count = 100);
    void sendPing();

signals:
    void connected();
    void disconnected();
    void connectionError(const QString& errorMessage);
    
    void authenticationResult(bool success, const QString& message);
    void gameStarted(const QJsonObject& gameData);
    void gameStateUpdated(const QJsonObject& gameState);
    void moveResult(bool success, const QString& message);
    void gameOver(const QJsonObject& gameOverData);
    void moveRecommendationsReceived(const QJsonArray& recommendations);
    void matchmakingStatus(const QJsonObject& statusData);
    void gameHistoryReceived(const QJsonArray& gameHistory);
    void gameAnalysisReceived(const QJsonObject& analysis);
    void leaderboardReceived(const QJsonObject& leaderboard);
    void errorReceived(const QString& errorMessage);
    void chatMessageReceived(const QString& sender, const QString& message);
    void drawOfferReceived(const QString& offeredBy);
    void drawResponseReceived(bool accepted);

private slots:
    void onConnected();
    void onDisconnected();
    void onError(QAbstractSocket::SocketError socketError);
    void onReadyRead();
    void onPingTimer();

private:
    Logger* logger;
    QTcpSocket* socket;
    QTimer* pingTimer;
    QByteArray buffer;
    
    void sendMessage(const QJsonObject& message);
    void processMessage(const QJsonObject& message);
    
    // Helper methods for processing different message types
    void processAuthenticationResult(const QJsonObject& data);
    void processGameStart(const QJsonObject& data);
    void processGameState(const QJsonObject& data);
    void processMoveResult(const QJsonObject& data);
    void processGameOver(const QJsonObject& data);
    void processMoveRecommendations(const QJsonObject& data);
    void processMatchmakingStatus(const QJsonObject& data);
    void processGameHistoryResponse(const QJsonObject& data);
    void processGameAnalysisResponse(const QJsonObject& data);
    void processLeaderboardResponse(const QJsonObject& data);
    void processError(const QJsonObject& data);
    void processChat(const QJsonObject& data);
    void processDrawOffer(const QJsonObject& data);
    void processDrawResponse(const QJsonObject& data);
};

/**
 * @brief Class for managing audio playback
 */
class AudioManager : public QObject {
    Q_OBJECT
    
public:
    enum class SoundEffect {
        MOVE,
        CAPTURE,
        CHECK,
        CHECKMATE,
        CASTLE,
        PROMOTION,
        GAME_START,
        GAME_END,
        ERROR,
        NOTIFICATION
    };
    
    AudioManager(QObject* parent = nullptr);
    ~AudioManager();
    
    void playSoundEffect(SoundEffect effect);
    void playBackgroundMusic(bool play);
    
    void setSoundEffectsEnabled(bool enabled);
    bool areSoundEffectsEnabled() const;
    
    void setBackgroundMusicEnabled(bool enabled);
    bool isBackgroundMusicEnabled() const;
    
    void setSoundEffectVolume(int volume); // 0-100
    int getSoundEffectVolume() const;
    
    void setBackgroundMusicVolume(int volume); // 0-100
    int getBackgroundMusicVolume() const;

private:
    bool soundEffectsEnabled;
    bool backgroundMusicEnabled;
    int soundEffectVolume;
    int backgroundMusicVolume;
    
    QMap<SoundEffect, QString> soundEffectPaths;
    QMediaPlayer* musicPlayer;
    QAudioOutput* musicOutput;
    
    void loadSoundEffects();
};

/**
 * @brief Class for managing themes and visual appearance
 */
class ThemeManager : public QObject {
    Q_OBJECT
    
public:
    enum class Theme {
        LIGHT,
        DARK,
        CUSTOM
    };
    
    enum class BoardTheme {
        CLASSIC,
        WOOD,
        MARBLE,
        BLUE,
        GREEN,
        CUSTOM
    };
    
    enum class PieceTheme {
        CLASSIC,
        MODERN,
        SIMPLE,
        FANCY,
        CUSTOM
    };
    
    ThemeManager(QObject* parent = nullptr);
    ~ThemeManager();
    
    void setTheme(Theme theme);
    Theme getTheme() const;
    
    void setBoardTheme(BoardTheme theme);
    BoardTheme getBoardTheme() const;
    
    void setPieceTheme(PieceTheme theme);
    PieceTheme getPieceTheme() const;
    
    QColor getLightSquareColor() const;
    QColor getDarkSquareColor() const;
    QColor getHighlightColor() const;
    QColor getLastMoveHighlightColor() const;
    QColor getCheckHighlightColor() const;
    
    QString getPieceThemePath() const;
    
    void setCustomLightSquareColor(const QColor& color);
    void setCustomDarkSquareColor(const QColor& color);
    void setCustomHighlightColor(const QColor& color);
    void setCustomLastMoveHighlightColor(const QColor& color);
    void setCustomCheckHighlightColor(const QColor& color);
    
    void setCustomPieceThemePath(const QString& path);
    
    QColor getTextColor() const;
    QColor getBackgroundColor() const;
    QColor getPrimaryColor() const;
    QColor getSecondaryColor() const;
    QColor getAccentColor() const;
    
    QString getStyleSheet() const;

signals:
    void themeChanged();
    void boardThemeChanged();
    void pieceThemeChanged();

private:
    Theme theme;
    BoardTheme boardTheme;
    PieceTheme pieceTheme;
    
    QColor customLightSquareColor;
    QColor customDarkSquareColor;
    QColor customHighlightColor;
    QColor customLastMoveHighlightColor;
    QColor customCheckHighlightColor;
    
    QString customPieceThemePath;
    
    void loadThemeSettings();
    void saveThemeSettings();
    
    QColor getLightSquareColorForTheme(BoardTheme theme) const;
    QColor getDarkSquareColorForTheme(BoardTheme theme) const;
    QString getPieceThemePathForTheme(PieceTheme theme) const;
};

/**
 * @brief Class representing a chess piece on the board
 */
class ChessPieceItem : public QGraphicsItem {
public:
    ChessPieceItem(PieceType type, PieceColor color, ThemeManager* themeManager, int squareSize);
    ~ChessPieceItem();
    
    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;
    
    PieceType getType() const;
    PieceColor getColor() const;
    
    void setSquareSize(int size);
    int getSquareSize() const;
    
    void updateTheme();

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;

private:
    PieceType type;
    PieceColor color;
    ThemeManager* themeManager;
    int squareSize;
    QSvgRenderer renderer;
    QPointF dragStartPosition;
    
    void loadSvg();
};

/**
 * @brief Widget for displaying and interacting with the chess board
 */
class ChessBoardWidget : public QGraphicsView {
    Q_OBJECT
    
public:
    ChessBoardWidget(ThemeManager* themeManager, AudioManager* audioManager, QWidget* parent = nullptr);
    ~ChessBoardWidget();
    
    void resetBoard();
    void setupInitialPosition();
    
    void setPiece(const Position& pos, PieceType type, PieceColor color);
    void removePiece(const Position& pos);
    void movePiece(const Position& from, const Position& to, bool animate = true);
    
    void setSquareSize(int size);
    int getSquareSize() const;
    
    void setFlipped(bool flipped);
    bool isFlipped() const;
    
    void highlightSquare(const Position& pos, const QColor& color);
    void clearHighlights();
    
    void highlightLastMove(const Position& from, const Position& to);
    void highlightCheck(const Position& kingPos);
    
    void setPlayerColor(PieceColor color);
    PieceColor getPlayerColor() const;
    
    void setInteractive(bool interactive);
    bool isInteractive() const;
    
    void showMoveHints(const QVector<Position>& positions);
    void clearMoveHints();
    
    void setCurrentGameId(const QString& gameId);
    QString getCurrentGameId() const;
    
    ChessPieceItem* getPieceAt(const Position& pos) const;
    Position getPositionAt(const QPointF& scenePos) const;
    
    void updateTheme();
    
    // For promotion dialog
    void showPromotionDialog(const Position& from, const Position& to, PieceColor color);

signals:
    void moveRequested(const QString& gameId, const ChessMove& move);
    void squareClicked(const Position& pos);

protected:
    void resizeEvent(QResizeEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dropEvent(QDropEvent* event) override;

private slots:
    void onPromotionSelected(PieceType promotionType);

private:
    ThemeManager* themeManager;
    AudioManager* audioManager;
    QGraphicsScene* scene;
    int squareSize;
    bool flipped;
    PieceColor playerColor;
    bool interactive;
    QString currentGameId;
    
    Position selectedPosition;
    Position dragStartPosition;
    
    QVector<QGraphicsRectItem*> highlightItems;
    QVector<QGraphicsEllipseItem*> hintItems;
    
    ChessPieceItem* pieces[8][8];
    
    void setupBoard();
    void updateBoardSize();
    void createSquares();
    
    Position boardToLogical(const Position& pos) const;
    Position logicalToBoard(const Position& pos) const;
    
    void startDrag(const Position& pos);
    void handleDrop(const Position& pos);
    
    void animatePieceMovement(ChessPieceItem* piece, const QPointF& startPos, const QPointF& endPos);
};

/**
 * @brief Widget for displaying move history
 */
class MoveHistoryWidget : public QWidget {
    Q_OBJECT
    
public:
    MoveHistoryWidget(QWidget* parent = nullptr);
    ~MoveHistoryWidget();
    
    void clear();
    void addMove(int moveNumber, const QString& whiteMoveNotation, const QString& blackMoveNotation = QString());
    void updateLastMove(const QString& moveNotation);
    
    void setCurrentMoveIndex(int index);
    int getCurrentMoveIndex() const;
    
    int getMoveCount() const;

signals:
    void moveSelected(int index);

private:
    QTableWidget* moveTable;
    int currentMoveIndex;
    
    void setupUI();
};

/**
 * @brief Widget for displaying captured pieces
 */
class CapturedPiecesWidget : public QWidget {
    Q_OBJECT
    
public:
    CapturedPiecesWidget(ThemeManager* themeManager, QWidget* parent = nullptr);
    ~CapturedPiecesWidget();
    
    void clear();
    void addCapturedPiece(PieceType type, PieceColor color);
    void updateTheme();
    
    void setMaterialAdvantage(int advantage);
    int getMaterialAdvantage() const;

private:
    ThemeManager* themeManager;
    QLabel* whiteCapturedLabel;
    QLabel* blackCapturedLabel;
    QLabel* materialAdvantageLabel;
    
    QVector<PieceType> whiteCapturedPieces;
    QVector<PieceType> blackCapturedPieces;
    int materialAdvantage;
    
    void setupUI();
    void updateDisplay();
    
    int getPieceValue(PieceType type) const;
    QString getPieceSymbol(PieceType type, PieceColor color) const;
};

/**
 * @brief Widget for displaying game timer
 */
class GameTimerWidget : public QWidget {
    Q_OBJECT
    
public:
    GameTimerWidget(QWidget* parent = nullptr);
    ~GameTimerWidget();
    
    void setWhiteTime(qint64 milliseconds);
    void setBlackTime(qint64 milliseconds);
    
    void setActiveColor(PieceColor color);
    PieceColor getActiveColor() const;
    
    void start();
    void stop();
    void reset();
    
    void setTimeControl(TimeControlType timeControl);
    TimeControlType getTimeControl() const;

private slots:
    void updateActiveTimer();

private:
    QLabel* whiteTimerLabel;
    QLabel* blackTimerLabel;
    QProgressBar* whiteProgressBar;
    QProgressBar* blackProgressBar;
    
    qint64 whiteTimeMs;
    qint64 blackTimeMs;
    PieceColor activeColor;
    TimeControlType timeControl;
    
    QTimer* timer;
    QDateTime lastUpdateTime;
    
    void setupUI();
    QString formatTime(qint64 milliseconds) const;
    qint64 getInitialTimeForControl(TimeControlType control) const;
    void updateProgressBars();
};

/**
 * @brief Widget for displaying game analysis
 */
class AnalysisWidget : public QWidget {
    Q_OBJECT
    
public:
    AnalysisWidget(QWidget* parent = nullptr);
    ~AnalysisWidget();
    
    void clear();
    void setAnalysisData(const QJsonObject& analysis);
    void setMoveRecommendations(const QJsonArray& recommendations);
    
    void setShowEvaluation(bool show);
    bool isShowingEvaluation() const;
    
    void setShowRecommendations(bool show);
    bool isShowingRecommendations() const;

signals:
    void moveSelected(const ChessMove& move);
    void requestAnalysis(bool stockfish);

private:
    QTabWidget* tabWidget;
    QWidget* evaluationTab;
    QWidget* recommendationsTab;
    QWidget* mistakesTab;
    
    QtCharts::QChartView* evaluationChartView;
    QTableWidget* recommendationsTable;
    QTableWidget* mistakesTable;
    
    QPushButton* analyzeButton;
    QPushButton* stockfishButton;
    
    bool showEvaluation;
    bool showRecommendations;
    
    void setupUI();
    void createEvaluationChart(const QJsonArray& moveAnalysis);
    void populateRecommendationsTable(const QJsonArray& recommendations);
    void populateMistakesTable(const QJsonObject& mistakes);
};

/**
 * @brief Widget for displaying player profile
 */
class ProfileWidget : public QWidget {
    Q_OBJECT
    
public:
    ProfileWidget(QWidget* parent = nullptr);
    ~ProfileWidget();
    
    void setPlayerData(const QJsonObject& playerData);
    void clear();

signals:
    void gameSelected(const QString& gameId);

private:
    QLabel* usernameLabel;
    QLabel* ratingLabel;
    QLabel* winsLabel;
    QLabel* lossesLabel;
    QLabel* drawsLabel;
    QLabel* winRateLabel;
    
    QtCharts::QChartView* statsChartView;
    QTableWidget* recentGamesTable;
    
    void setupUI();
    void createStatsChart(int wins, int losses, int draws);
    void populateRecentGamesTable(const QJsonArray& games);
};

/**
 * @brief Widget for displaying leaderboard
 */
class LeaderboardWidget : public QWidget {
    Q_OBJECT
    
public:
    LeaderboardWidget(QWidget* parent = nullptr);
    ~LeaderboardWidget();
    
    void setLeaderboardData(const QJsonObject& leaderboard);
    void clear();
    
    void setPlayerRanks(const QJsonObject& ranks);

signals:
    void playerSelected(const QString& username);
    void requestAllPlayers(bool all);

private:
    QTabWidget* tabWidget;
    QTableWidget* ratingTable;
    QTableWidget* winsTable;
    QTableWidget* winRateTable;
    
    QLabel* yourRatingRankLabel;
    QLabel* yourWinsRankLabel;
    QLabel* yourWinRateRankLabel;
    
    QPushButton* showAllButton;
    QLabel* totalPlayersLabel;
    
    void setupUI();
    void populateTable(QTableWidget* table, const QJsonArray& data);
    void highlightPlayer(QTableWidget* table, const QString& username);
};

/**
 * @brief Widget for matchmaking
 */
class MatchmakingWidget : public QWidget {
    Q_OBJECT
    
public:
    MatchmakingWidget(QWidget* parent = nullptr);
    ~MatchmakingWidget();
    
    void setMatchmakingStatus(const QJsonObject& status);
    void clear();
    
    bool isInQueue() const;

signals:
    void requestMatchmaking(bool join, TimeControlType timeControl);

private slots:
    void onJoinQueueClicked();
    void onLeaveQueueClicked();

private:
    QComboBox* timeControlComboBox;
    QPushButton* joinQueueButton;
    QPushButton* leaveQueueButton;
    QLabel* statusLabel;
    QProgressBar* queueProgressBar;
    QLabel* queueTimeLabel;
    QLabel* queueSizeLabel;
    
    bool inQueue;
    QTimer* queueTimer;
    QDateTime queueStartTime;
    
    void setupUI();
    void updateQueueTime();
    TimeControlType getSelectedTimeControl() const;
};

/**
 * @brief Widget for displaying game history
 */
class GameHistoryWidget : public QWidget {
    Q_OBJECT
    
public:
    GameHistoryWidget(QWidget* parent = nullptr);
    ~GameHistoryWidget();
    
    void setGameHistoryData(const QJsonArray& gameHistory);
    void clear();

signals:
    void gameSelected(const QString& gameId);
    void requestGameHistory();

private:
    QTableWidget* gamesTable;
    QPushButton* refreshButton;
    QComboBox* filterComboBox;
    
    void setupUI();
    void populateGamesTable(const QJsonArray& games);
};

/**
 * @brief Dialog for piece promotion
 */
class PromotionDialog : public QDialog {
    Q_OBJECT
    
public:
    PromotionDialog(PieceColor color, ThemeManager* themeManager, QWidget* parent = nullptr);
    ~PromotionDialog();
    
    PieceType getSelectedPieceType() const;

signals:
    void pieceSelected(PieceType type);

private:
    PieceType selectedType;
    ThemeManager* themeManager;
    PieceColor color;
    
    void setupUI();
    void createPieceButton(PieceType type, const QString& label, QHBoxLayout* layout);
};

/**
 * @brief Dialog for login
 */
class LoginDialog : public QDialog {
    Q_OBJECT
    
public:
    LoginDialog(QWidget* parent = nullptr);
    ~LoginDialog();
    
    QString getUsername() const;
    QString getPassword() const;
    bool isRegistering() const;

signals:
    void loginRequested(const QString& username, const QString& password, bool isRegistering);

private slots:
    void onLoginClicked();
    void onRegisterClicked();
    void onTogglePasswordVisibility();

private:
    QLineEdit* usernameEdit;
    QLineEdit* passwordEdit;
    QPushButton* loginButton;
    QPushButton* registerButton;
    QPushButton* togglePasswordButton;
    QLabel* statusLabel;
    
    bool registering;
    
    void setupUI();
};

/**
 * @brief Dialog for settings
 */
class SettingsDialog : public QDialog {
    Q_OBJECT
    
public:
    SettingsDialog(ThemeManager* themeManager, AudioManager* audioManager, QWidget* parent = nullptr);
    ~SettingsDialog();

signals:
    void settingsChanged();

private slots:
    void onThemeChanged(int index);
    void onBoardThemeChanged(int index);
    void onPieceThemeChanged(int index);
    void onSoundEffectsToggled(bool enabled);
    void onMusicToggled(bool enabled);
    void onSoundVolumeChanged(int value);
    void onMusicVolumeChanged(int value);
    void onCustomColorsClicked();
    void onResetToDefaultsClicked();

private:
    ThemeManager* themeManager;
    AudioManager* audioManager;
    
    QComboBox* themeComboBox;
    QComboBox* boardThemeComboBox;
    QComboBox* pieceThemeComboBox;
    
    QCheckBox* soundEffectsCheckBox;
    QCheckBox* musicCheckBox;
    QSlider* soundVolumeSlider;
    QSlider* musicVolumeSlider;
    
    QPushButton* customColorsButton;
    QPushButton* resetButton;
    
    void setupUI();
    void loadSettings();
    void saveSettings();
};

/**
 * @brief Class for managing game state
 */
class GameManager : public QObject {
    Q_OBJECT
    
public:
    GameManager(NetworkManager* networkManager, Logger* logger, QObject* parent = nullptr);
    ~GameManager();
    
    void startNewGame(const QJsonObject& gameData);
    void updateGameState(const QJsonObject& gameState);
    void endGame(const QJsonObject& gameOverData);
    
    QString getCurrentGameId() const;
    PieceColor getPlayerColor() const;
    bool isGameActive() const;
    
    void makeMove(const ChessMove& move);
    void offerDraw();
    void respondToDraw(bool accept);
    void resign();
    
    const QJsonObject& getCurrentGameState() const;
    QVector<ChessMove> getMoveHistory() const;
    
    void setMoveRecommendations(const QJsonArray& recommendations);
    QJsonArray getMoveRecommendations() const;

signals:
    void gameStarted(const QJsonObject& gameData);
    void gameStateUpdated(const QJsonObject& gameState);
    void gameEnded(const QJsonObject& gameOverData);
    void moveHistoryUpdated(const QVector<ChessMove>& moves);
    void moveRecommendationsUpdated(const QJsonArray& recommendations);

private:
    NetworkManager* networkManager;
    Logger* logger;
    
    QString currentGameId;
    PieceColor playerColor;
    bool gameActive;
    
    QJsonObject currentGameState;
    QVector<ChessMove> moveHistory;
    QJsonArray moveRecommendations;
    
    void parseMoveHistory(const QJsonArray& moveHistoryArray);
};

/**
 * @brief Main window for the chess client
 */
class MPChessClient : public QMainWindow {
    Q_OBJECT
    
public:
    MPChessClient(QWidget* parent = nullptr);
    ~MPChessClient();
    
    bool connectToServer(const QString& host, int port);
    void disconnectFromServer();

protected:
    void closeEvent(QCloseEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private slots:
    // Network slots
    void onConnected();
    void onDisconnected();
    void onConnectionError(const QString& errorMessage);
    void onAuthenticationResult(bool success, const QString& message);
    
    // Game management slots
    void onGameStarted(const QJsonObject& gameData);
    void onGameStateUpdated(const QJsonObject& gameState);
    void onGameOver(const QJsonObject& gameOverData);
    void onMoveResult(bool success, const QString& message);
    void onMoveRecommendationsReceived(const QJsonArray& recommendations);
    
    // UI interaction slots
    void onMoveRequested(const QString& gameId, const ChessMove& move);
    void onSquareClicked(const Position& pos);
    void onResignClicked();
    void onDrawOfferClicked();
    void onDrawOfferReceived(const QString& offeredBy);
    void onDrawResponseReceived(bool accepted);
    
    // Menu action slots
    void onConnectAction();
    void onDisconnectAction();
    void onSettingsAction();
    void onExitAction();
    void onFlipBoardAction();
    void onShowAnalysisAction();
    void onShowChatAction();
    void onAboutAction();
    
    // Navigation slots
    void onHomeTabSelected();
    void onPlayTabSelected();
    void onAnalysisTabSelected();
    void onProfileTabSelected();
    void onLeaderboardTabSelected();
    
    // Matchmaking slots
    void onMatchmakingStatusReceived(const QJsonObject& statusData);
    void onRequestMatchmaking(bool join, TimeControlType timeControl);
    
    // History and analysis slots
    void onGameHistoryReceived(const QJsonArray& gameHistory);
    void onGameAnalysisReceived(const QJsonObject& analysis);
    void onGameSelected(const QString& gameId);
    void onRequestGameHistory();
    void onRequestGameAnalysis(bool stockfish);
    
    // Leaderboard slots
    void onLeaderboardReceived(const QJsonObject& leaderboard);
    void onRequestLeaderboard(bool allPlayers);

private:
    Ui::MPChessClient* ui;
    
    // Core components
    Logger* logger;
    NetworkManager* networkManager;
    GameManager* gameManager;
    ThemeManager* themeManager;
    AudioManager* audioManager;
    
    // UI components
    QStackedWidget* mainStack;
    LoginDialog* loginDialog;
    
    // Game UI
    ChessBoardWidget* boardWidget;
    MoveHistoryWidget* moveHistoryWidget;
    CapturedPiecesWidget* capturedPiecesWidget;
    GameTimerWidget* gameTimerWidget;
    AnalysisWidget* analysisWidget;
    
    // Other UI components
    ProfileWidget* profileWidget;
    LeaderboardWidget* leaderboardWidget;
    MatchmakingWidget* matchmakingWidget;
    GameHistoryWidget* gameHistoryWidget;
    
    // Status indicators
    QLabel* connectionStatusLabel;
    QLabel* gameStatusLabel;
    
    // Chat
    QTextEdit* chatDisplay;
    QLineEdit* chatInput;
    
    // Game replay controls
    QSlider* replaySlider;
    QPushButton* replayPrevButton;
    QPushButton* replayPlayButton;
    QPushButton* replayNextButton;
    bool replayMode;
    int currentReplayIndex;
    
    void setupUI();
    void createMenus();
    void createStatusBar();
    void createGameUI();
    void createProfileUI();
    void createLeaderboardUI();
    void createMatchmakingUI();
    void createHistoryUI();
    
    void updateBoardFromGameState(const QJsonObject& gameState);
    void updateCapturedPieces(const QJsonObject& gameState);
    void updateMoveHistory(const QJsonObject& gameState);
    void updateTimers(const QJsonObject& gameState);
    
    void showLoginDialog();
    void showMessage(const QString& message, bool error = false);
    
    void enterReplayMode(const QVector<ChessMove>& moves);
    void exitReplayMode();
    void updateReplayControls();
    
    void saveSettings();
    void loadSettings();
    
    void updateTheme();
};

#endif // MPCHESSCLIENT_H