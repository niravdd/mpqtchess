// MPChessServer.h

#ifndef MP_CHESS_SERVER_H
#define MP_CHESS_SERVER_H

#include <QCoreApplication>
#include <QTcpServer>
#include <QTcpSocket>
#include <QUdpSocket>
#include <QTimer>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QDir>
#include <QMap>
#include <QSet>
#include <QQueue>
#include <QRandomGenerator>
#include <QCryptographicHash>
#include <QMutex>
#include <QThread>
#include <QThreadPool>
#include <QRunnable>
#include <QSslSocket>
#include <QSslConfiguration>
#include <QSslKey>
#include <QSslCertificate>
#include <QHostAddress>
#include <QNetworkInterface>
#include <QCommandLineParser>
#include <QLoggingCategory>
#include <QProcess>

#include <array>
#include <vector>
#include <string>
#include <unordered_map>
#include <functional>
#include <memory>
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

// Forward declarations
class ChessGame;
class ChessPlayer;
class ChessMove;
class ChessBoard;
class ChessPiece;
class ChessAI;
class ChessMatchmaker;
class ChessRatingSystem;
class ChessAnalysisEngine;
class ChessLogger;
class ChessAuthenticator;
class ChessSerializer;

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
 * @brief Enum representing the status of a move validation
 */
enum class MoveValidationStatus {
    VALID,
    INVALID_PIECE,
    INVALID_DESTINATION,
    INVALID_PATH,
    KING_IN_CHECK,
    WRONG_TURN,
    GAME_OVER
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
    std::string toAlgebraic() const {
        if (!isValid()) return "invalid";
        return std::string(1, 'a' + col) + std::to_string(row + 1);
    }
    
    // Create from algebraic notation (e.g., "e4")
    static Position fromAlgebraic(const std::string& algebraic) {
        if (algebraic.length() != 2) return Position();
        int col = algebraic[0] - 'a';
        int row = algebraic[1] - '1';
        if (col < 0 || col > 7 || row < 0 || row > 7) return Position();
        return Position(row, col);
    }
};

/**
 * @brief Class representing a chess piece
 */
class ChessPiece {
public:
    ChessPiece(PieceType type, PieceColor color);
    virtual ~ChessPiece() = default;
    
    PieceType getType() const;
    PieceColor getColor() const;
    bool hasMoved() const;
    void setMoved(bool moved);
    
    // Get possible moves for this piece from the given position
    virtual std::vector<Position> getPossibleMoves(const Position& pos, const ChessBoard& board) const = 0;
    
    // Get the character representation of this piece for ASCII display
    char getAsciiChar() const;
    
    // Clone this piece
    virtual std::unique_ptr<ChessPiece> clone() const = 0;

protected:
    PieceType type;
    PieceColor color;
    bool moved;
};

/**
 * @brief Class representing a chess move
 */
class ChessMove {
public:
    ChessMove();
    ChessMove(const Position& from, const Position& to, PieceType promotionType = PieceType::EMPTY);
    
    Position getFrom() const;
    Position getTo() const;
    PieceType getPromotionType() const;
    void setPromotionType(PieceType type);
    
    // Convert to algebraic notation (e.g., "e2e4" or "e7e8q" for promotion)
    std::string toAlgebraic() const;
    
    // Create from algebraic notation
    static ChessMove fromAlgebraic(const std::string& algebraic);
    
    // Get the standard chess notation (e.g., "e4", "Nf3", "O-O")
    std::string toStandardNotation(const ChessBoard& board) const;
    
    bool operator==(const ChessMove& other) const;
    bool operator!=(const ChessMove& other) const;

private:
    Position from;
    Position to;
    PieceType promotionType;
};

/**
 * @brief Class representing a chess board
 */
class ChessBoard {
public:
    ChessBoard();
    ~ChessBoard() = default;
    
    // Initialize the board with pieces in starting positions
    void initialize();
    
    // Get the piece at the given position
    const ChessPiece* getPiece(const Position& pos) const;
    
    // Move a piece from one position to another
    MoveValidationStatus movePiece(const ChessMove& move, bool validateOnly = false);
    
    // Check if the given position is under attack by the given color
    bool isUnderAttack(const Position& pos, PieceColor attackerColor) const;
    
    // Check if the king of the given color is in check
    bool isInCheck(PieceColor color) const;
    
    // Check if the king of the given color is in checkmate
    bool isInCheckmate(PieceColor color) const;
    
    // Check if the game is in stalemate for the given color
    bool isInStalemate(PieceColor color) const;
    
    // Get all valid moves for the given color
    std::vector<ChessMove> getAllValidMoves(PieceColor color) const;
    
    // Get the position of the king of the given color
    Position getKingPosition(PieceColor color) const;
    
    // Check if the move is a castling move
    bool isCastlingMove(const ChessMove& move) const;
    
    // Check if the move is an en passant capture
    bool isEnPassantCapture(const ChessMove& move) const;
    
    // Get the position of the pawn that can be captured by en passant
    Position getEnPassantTarget() const;
    
    // Set the position of the pawn that can be captured by en passant
    void setEnPassantTarget(const Position& pos);
    
    // Get the ASCII representation of the board
    std::string getAsciiBoard() const;
    
    // Clone this board
    std::unique_ptr<ChessBoard> clone() const;
    
    // Get the current turn
    PieceColor getCurrentTurn() const;
    
    // Set the current turn
    void setCurrentTurn(PieceColor color);
    
    // Get the move history
    const std::vector<ChessMove>& getMoveHistory() const;
    
    // Get the captured pieces
    const std::vector<PieceType>& getCapturedPieces(PieceColor color) const;
    
    // Check if the game is over
    bool isGameOver() const;
    
    // Get the result of the game
    GameResult getGameResult() const;
    
    // Check if a draw can be claimed due to threefold repetition
    bool canClaimThreefoldRepetition() const;
    
    // Check if a draw can be claimed due to the fifty-move rule
    bool canClaimFiftyMoveRule() const;
    
    // Check if there is insufficient material for checkmate
    bool hasInsufficientMaterial() const;

private:
    std::array<std::array<std::unique_ptr<ChessPiece>, 8>, 8> board;
    PieceColor currentTurn;
    Position enPassantTarget;
    std::vector<ChessMove> moveHistory;
    std::vector<PieceType> capturedWhitePieces;
    std::vector<PieceType> capturedBlackPieces;
    int halfMoveClock;  // For fifty-move rule
    std::vector<std::string> boardStates;  // For threefold repetition
    
    // Execute a castling move
    void executeCastlingMove(const ChessMove& move);
    
    // Execute an en passant capture
    void executeEnPassantCapture(const ChessMove& move);
    
    // Update the state after a move
    void updateStateAfterMove(const ChessMove& move, const ChessPiece* capturedPiece);
    
    // Check if the move would leave the king in check
    bool wouldLeaveInCheck(const ChessMove& move, PieceColor color) const;
    
    // Get the current board state as a string for repetition detection
    std::string getBoardStateString() const;
};

/**
 * @brief Class representing a chess player
 */
class ChessPlayer {
public:
    ChessPlayer(const std::string& username, QTcpSocket* socket = nullptr);
    ~ChessPlayer();
    
    std::string getUsername() const;
    int getRating() const;
    void setRating(int rating);
    PieceColor getColor() const;
    void setColor(PieceColor color);
    QTcpSocket* getSocket() const;
    void setSocket(QTcpSocket* socket);
    
    // Get the player's game statistics
    int getGamesPlayed() const;
    int getWins() const;
    int getLosses() const;
    int getDraws() const;
    
    // Update the player's statistics after a game
    void updateStats(GameResult result);
    
    // Get the player's remaining time in milliseconds
    qint64 getRemainingTime() const;
    
    // Set the player's remaining time in milliseconds
    void setRemainingTime(qint64 time);
    
    // Decrement the player's remaining time
    void decrementTime(qint64 milliseconds);
    
    // Check if the player is a bot
    bool isBot() const;
    
    // Set whether the player is a bot
    void setBot(bool isBot);
    
    // Get the player's game history IDs
    const std::vector<std::string>& getGameHistory() const;
    
    // Add a game to the player's history
    void addGameToHistory(const std::string& gameId);
    
    // Serialize the player data to JSON
    QJsonObject toJson() const;
    
    // Deserialize the player data from JSON
    static ChessPlayer fromJson(const QJsonObject& json);

private:
    std::string username;
    int rating;
    PieceColor color;
    QTcpSocket* socket;
    int gamesPlayed;
    int wins;
    int losses;
    int draws;
    qint64 remainingTime;
    bool bot;
    std::vector<std::string> gameHistory;
};

/**
 * @brief Class representing a chess game
 */
class ChessGame {
public:
    ChessGame(ChessPlayer* whitePlayer, ChessPlayer* blackPlayer, 
              const std::string& gameId, TimeControlType timeControl);
    ~ChessGame();
    
    std::string getGameId() const;
    ChessPlayer* getWhitePlayer() const;
    ChessPlayer* getBlackPlayer() const;
    ChessPlayer* getCurrentPlayer() const;
    ChessPlayer* getOpponentPlayer(const ChessPlayer* player) const;
    ChessBoard* getBoard() const;
    GameResult getResult() const;
    void setResult(GameResult result);
    TimeControlType getTimeControl() const;
    
    // Process a move from a player
    MoveValidationStatus processMove(ChessPlayer* player, const ChessMove& move);
    
    // Start the game
    void start();
    
    // End the game
    void end(GameResult result);
    
    // Check if the game is over
    bool isOver() const;
    
    // Get the game state as JSON
    QJsonObject getGameStateJson() const;
    
    // Get the game history as JSON
    QJsonObject getGameHistoryJson() const;
    
    // Get the time taken for each move
    const std::vector<std::pair<ChessMove, qint64>>& getMoveTimings() const;
    
    // Get the ASCII representation of the current board state
    std::string getBoardAscii() const;
    
    // Get move recommendations for a player
    std::vector<std::pair<ChessMove, double>> getMoveRecommendations(ChessPlayer* player) const;
    
    // Handle a draw offer
    bool handleDrawOffer(ChessPlayer* player);
    
    // Handle a draw response
    void handleDrawResponse(ChessPlayer* player, bool accepted);
    
    // Handle a resignation
    void handleResignation(ChessPlayer* player);
    
    // Update the game timers
    void updateTimers();
    
    // Check if a player has run out of time
    bool hasPlayerTimedOut(ChessPlayer* player) const;
    
    // Serialize the game state
    QJsonObject serialize() const;
    
    // Deserialize the game state
    static std::unique_ptr<ChessGame> deserialize(const QJsonObject& json, 
                                                 ChessPlayer* whitePlayer, 
                                                 ChessPlayer* blackPlayer);

private:
    std::string gameId;
    ChessPlayer* whitePlayer;
    ChessPlayer* blackPlayer;
    std::unique_ptr<ChessBoard> board;
    GameResult result;
    TimeControlType timeControl;
    QDateTime startTime;
    QDateTime endTime;
    QDateTime lastMoveTime;
    std::vector<std::pair<ChessMove, qint64>> moveTimings;
    bool drawOffered;
    ChessPlayer* drawOfferingPlayer;
    
    // Initialize the time control
    void initializeTimeControl();
    
    // Update the player's remaining time after a move
    void updatePlayerTime(ChessPlayer* player);
};

/**
 * @brief Class for chess AI player
 */
class ChessAI {
public:
    ChessAI(int skillLevel = 5);
    ~ChessAI() = default;
    
    // Get the best move for the current board state
    ChessMove getBestMove(const ChessBoard& board, PieceColor color);
    
    // Set the skill level (1-10)
    void setSkillLevel(int level);
    
    // Get the skill level
    int getSkillLevel() const;
    
    // Evaluate a board position
    double evaluatePosition(const ChessBoard& board, PieceColor color) const;
    
    // Get move recommendations with evaluations
    std::vector<std::pair<ChessMove, double>> getMoveRecommendations(
        const ChessBoard& board, PieceColor color, int maxRecommendations = 5);

private:
    int skillLevel;
    
    // Minimax algorithm with alpha-beta pruning
    double minimax(const ChessBoard& board, int depth, double alpha, double beta, 
                  bool maximizingPlayer, PieceColor aiColor);
    
    // Get the search depth based on skill level
    int getSearchDepth() const;
    
    // Evaluate a single piece
    double evaluatePiece(const ChessPiece* piece, const Position& pos) const;
    
    // Piece-square tables for positional evaluation
    static const std::array<std::array<double, 8>, 8> pawnTable;
    static const std::array<std::array<double, 8>, 8> knightTable;
    static const std::array<std::array<double, 8>, 8> bishopTable;
    static const std::array<std::array<double, 8>, 8> rookTable;
    static const std::array<std::array<double, 8>, 8> queenTable;
    static const std::array<std::array<double, 8>, 8> kingMiddleGameTable;
    static const std::array<std::array<double, 8>, 8> kingEndGameTable;
};

/**
 * @brief Class for chess matchmaking
 */
class ChessMatchmaker {
public:
    ChessMatchmaker();
    ~ChessMatchmaker() = default;
    
    // Add a player to the matchmaking queue
    void addPlayer(ChessPlayer* player);
    
    // Remove a player from the matchmaking queue
    void removePlayer(ChessPlayer* player);
    
    // Try to match players
    std::vector<std::pair<ChessPlayer*, ChessPlayer*>> matchPlayers();
    
    // Check for timed out players and match them with bots
    std::vector<ChessPlayer*> checkTimeouts(int timeoutSeconds = 60);
    
    // Get the number of players in the queue
    int getQueueSize() const;
    
    // Clear the queue
    void clearQueue();

private:
    std::vector<ChessPlayer*> playerQueue;
    std::unordered_map<ChessPlayer*, QDateTime> queueTimes;
    
    // Find the best match for a player based on rating
    ChessPlayer* findBestMatch(ChessPlayer* player) const;
    
    // Calculate the rating difference score (lower is better)
    double getRatingDifferenceScore(int rating1, int rating2) const;
};

/**
 * @brief Class for chess rating system (Elo)
 */
class ChessRatingSystem {
public:
    ChessRatingSystem();
    ~ChessRatingSystem() = default;
    
    // Calculate the new ratings after a game
    std::pair<int, int> calculateNewRatings(int rating1, int rating2, GameResult result);
    
    // Calculate the expected score for a player
    double calculateExpectedScore(int rating1, int rating2) const;
    
    // Get the K-factor for a player based on rating and games played
    int getKFactor(int rating, int gamesPlayed) const;

private:
    static constexpr int DEFAULT_K_FACTOR = 32;
    static constexpr int EXPERIENCED_K_FACTOR = 24;
    static constexpr int MASTER_K_FACTOR = 16;
    static constexpr int GAMES_THRESHOLD = 30;
    static constexpr int MASTER_RATING_THRESHOLD = 2200;
};

/**
 * @brief Class for chess analysis engine
 */
class ChessAnalysisEngine {
public:
    ChessAnalysisEngine();
    ~ChessAnalysisEngine() = default;
    
    // Analyze a game and provide insights
    QJsonObject analyzeGame(const ChessGame& game);
    
    // Analyze a specific move
    QJsonObject analyzeMove(const ChessBoard& boardBefore, const ChessMove& move);
    
    // Get move recommendations
    std::vector<std::pair<ChessMove, double>> getMoveRecommendations(
        const ChessBoard& board, PieceColor color, int maxRecommendations = 5);
    
    // Identify blunders, mistakes, and inaccuracies
    QJsonObject identifyMistakes(const ChessGame& game);
    int countPlayerMistakes(const QJsonArray& mistakes, const QString& color);
    
    // Identify critical moments in the game
    QJsonObject identifyCriticalMoments(const ChessGame& game);
    
    // Generate a textual summary of the game
    std::string generateGameSummary(const ChessGame& game);

private:
    ChessAI analysisAI;
    
    // Evaluate a position deeply
    double evaluatePositionDeeply(const ChessBoard& board, PieceColor color);
    
    // Classify a move based on evaluation difference
    std::string classifyMove(double evalBefore, double evalAfter);
    
    // Check if a move is a capture
    bool isCapture(const ChessBoard& board, const ChessMove& move);
    
    // Check if a move puts the opponent in check
    bool putsInCheck(const ChessBoard& board, const ChessMove& move);
};

/**
 * @brief Class for connecting to and communicating with a Stockfish chess engine
 */
class StockfishConnector {
public:
    StockfishConnector(const std::string& enginePath, int skillLevel = 20, int depth = 15);
    ~StockfishConnector();
    
    // Initialize the connection to Stockfish
    bool initialize();
    
    // Check if the connector is initialized
    bool isInitialized() const;
    
    // Set the skill level (0-20)
    void setSkillLevel(int level);
    
    // Set the search depth
    void setDepth(int depth);
    
    // Set the position from a board
    void setPosition(const ChessBoard& board);
    
    // Get the best move for the current position
    ChessMove getBestMove();
    
    // Get move recommendations with evaluations
    std::vector<std::pair<ChessMove, double>> getMoveRecommendations(int maxRecommendations = 5);
    
    // Evaluate the current position
    double evaluatePosition();
    
    // Analyze a position deeply and return a detailed analysis
    QJsonObject analyzePosition(const ChessBoard& board);
    
    // Analyze a game and provide insights
    QJsonObject analyzeGame(const ChessGame& game);

private:
    std::string enginePath;
    int skillLevel;
    int depth;
    QProcess* process;
    bool initialized;
    
    // Send a command to Stockfish and wait for "readyok"
    void sendCommand(const std::string& command);
    
    // Send a command and get the output until a specific terminator is found
    std::string sendCommandAndGetOutput(const std::string& command, const std::string& terminator);
    
    // Parse the evaluation score from Stockfish output
    double parseEvaluation(const std::string& evalStr);
    
    // Convert a board to FEN notation
    std::string boardToFen(const ChessBoard& board);
    
    // Parse a move from Stockfish format to our format
    ChessMove parseStockfishMove(const std::string& moveStr, const ChessBoard& board);
};

/**
 * @brief Class for chess game serialization
 */
class ChessSerializer {
public:
    ChessSerializer();
    ~ChessSerializer() = default;
    
    // Serialize a game to JSON
    QJsonObject serializeGame(const ChessGame& game);
    
    // Deserialize a game from JSON
    std::unique_ptr<ChessGame> deserializeGame(const QJsonObject& json, 
                                             ChessPlayer* whitePlayer, 
                                             ChessPlayer* blackPlayer);
    
    // Save a game to a file
    bool saveGameToFile(const ChessGame& game, const std::string& filename);
    
    // Load a game from a file
    std::unique_ptr<ChessGame> loadGameFromFile(const std::string& filename,
                                              ChessPlayer* whitePlayer,
                                              ChessPlayer* blackPlayer);
    
    // Serialize a player to JSON
    QJsonObject serializePlayer(const ChessPlayer& player);
    
    // Deserialize a player from JSON
    std::unique_ptr<ChessPlayer> deserializePlayer(const QJsonObject& json);
    
    // Save player data to a file
    bool savePlayerToFile(const ChessPlayer& player, const std::string& filename);
    
    // Load player data from a file
    std::unique_ptr<ChessPlayer> loadPlayerFromFile(const std::string& filename);
    
    // Serialize a board to JSON
    QJsonObject serializeBoard(const ChessBoard& board);
    
    // Deserialize a board from JSON
    std::unique_ptr<ChessBoard> deserializeBoard(const QJsonObject& json);

private:
    // Helper methods for serialization/deserialization
    QJsonObject serializePiece(const ChessPiece* piece, const Position& pos);
    std::unique_ptr<ChessPiece> deserializePiece(const QJsonObject& json);
    QJsonObject serializeMove(const ChessMove& move);
    ChessMove deserializeMove(const QJsonObject& json);
};

/**
 * @brief Class for chess server logging
 */
class ChessLogger {
public:
    ChessLogger(const std::string& logFilePath);
    ~ChessLogger();
    
    // Log a message with a timestamp
    void log(const std::string& message, bool console = false);
    
    // Log an error with a timestamp
    void error(const std::string& message, bool console = true);
    
    // Log a warning with a timestamp
    void warning(const std::string& message, bool console = true);
    
    // Log a debug message with a timestamp
    void debug(const std::string& message, bool console = false);
    
    // Log a game state
    void logGameState(const ChessGame& game);
    
    // Log a player action
    void logPlayerAction(const ChessPlayer& player, const std::string& action);
    
    // Log a server event
    void logServerEvent(const std::string& event);
    
    // Log a network message
    void logNetworkMessage(const std::string& direction, const QJsonObject& message);
    
    // Set the log level
    void setLogLevel(int level);
    
    // Get the current log level
    int getLogLevel() const;
    
    // Flush the log to disk
    void flush();

private:
    std::ofstream logFile;
    int logLevel;
    std::mutex logMutex;
    
    // Get the current timestamp as a string
    std::string getCurrentTimestamp() const;
};

/**
 * @brief Class for player authentication
 */
class ChessAuthenticator {
public:
    ChessAuthenticator(const std::string& userDbPath);
    ~ChessAuthenticator();
    
    // Authenticate a player
    bool authenticatePlayer(const std::string& username, const std::string& password);
    
    // Register a new player
    bool registerPlayer(const std::string& username, const std::string& password);
    
    // Check if a username exists
    bool usernameExists(const std::string& username);
    
    // Get a player by username
    std::unique_ptr<ChessPlayer> getPlayer(const std::string& username);
    
    // Save a player's data
    bool savePlayer(const ChessPlayer& player);
    
    // Get all registered players
    std::vector<std::string> getAllPlayerUsernames();
    
    // Delete a player
    bool deletePlayer(const std::string& username);

private:
    std::string userDbPath;
    std::unordered_map<std::string, std::string> passwordCache;
    std::mutex authMutex;
    
    // Hash a password
    std::string hashPassword(const std::string& password, const std::string& salt = "");
    
    // Generate a random salt
    std::string generateSalt(int length = 16);
    
    // Load the password database
    void loadPasswordDb();
    
    // Save the password database
    void savePasswordDb();
    
    // Get the path to a player's data file
    std::string getPlayerFilePath(const std::string& username);
};

/**
 * @brief Class for managing player leaderboards
 */
class ChessLeaderboard {
public:
    ChessLeaderboard(const std::string& dataPath);
    ~ChessLeaderboard() = default;
    
    // Update the leaderboard with a player's data
    void updatePlayer(const ChessPlayer& player);
    
    // Get the top N players by rating (or all if count is -1)
    std::vector<std::pair<std::string, int>> getTopPlayersByRating(int count = 100);
    
    // Get the top N players by wins (or all if count is -1)
    std::vector<std::pair<std::string, int>> getTopPlayersByWins(int count = 100);
    
    // Get the top N players by win percentage (minimum 10 games) (or all if count is -1)
    std::vector<std::pair<std::string, double>> getTopPlayersByWinPercentage(int count = 100);
    
    // Get a player's rank by rating
    int getPlayerRatingRank(const std::string& username);
    
    // Get a player's rank by wins
    int getPlayerWinsRank(const std::string& username);
    
    // Get a player's rank by win percentage
    int getPlayerWinPercentageRank(const std::string& username);
    
    // Generate the leaderboard JSON (all players if count is -1)
    QJsonObject generateLeaderboardJson(int count = 100);
    
    // Refresh the leaderboard from player data files
    void refreshLeaderboard();

private:
    std::string dataPath;
    std::vector<std::tuple<std::string, int, int, int, int, double>> leaderboardData;  // username, rating, wins, losses, draws, win%
    std::mutex leaderboardMutex;
    
    // Load all player data
    void loadPlayerData();
    
    // Sort the leaderboard by different criteria
    void sortByRating();
    void sortByWins();
    void sortByWinPercentage();
};

/**
 * @brief Main class for the multiplayer chess server
 */
class MPChessServer : public QObject {
    Q_OBJECT
    Q_DISABLE_COPY(MPChessServer)

public:
    MPChessServer(QObject* parent = nullptr, const std::string& stockfishPath = "");
    ~MPChessServer();

    static MPChessServer* getInstance() { return mpChessServerInstance; }

    // Start the server on the specified port
    bool start(int port);
    
    // Stop the server
    void stop();
    
    // Check if the server is running
    bool isRunning() const;
    
    // Get the server port
    int getPort() const;
    
    // Get the number of connected clients
    int getConnectedClientCount() const;
    
    // Get the number of active games
    int getActiveGameCount() const;
    
    // Get the server uptime in seconds
    qint64 getUptime() const;
    
    // Get server statistics
    QJsonObject getServerStats() const;

    // Process a leaderboard request
    void processLeaderboardRequest(QTcpSocket* socket, const QJsonObject& data);

    // Making stockfishConnector & leaderboard public so it can be accessed by other classes
    std::unique_ptr<StockfishConnector> stockfishConnector;
    std::unique_ptr<ChessLeaderboard> leaderboard;

public slots:
    // Handle a new client connection
    void handleNewConnection();
    
    // Handle client disconnection
    void handleClientDisconnected();
    
    // Handle client data
    void handleClientData();
    
    // Handle matchmaking timer
    void handleMatchmakingTimer();
    
    // Handle game timer updates
    void handleGameTimerUpdate();
    
    // Handle server status updates
    void handleServerStatusUpdate();

    // Handle leaderboard refresh timer
    void handleLeaderboardRefresh();

private:
    static MPChessServer* mpChessServerInstance; 
    QTcpServer* server;
    std::unique_ptr<ChessLogger> logger;
    std::unique_ptr<ChessAuthenticator> authenticator;
    std::unique_ptr<ChessMatchmaker> matchmaker;
    std::unique_ptr<ChessRatingSystem> ratingSystem;
    std::unique_ptr<ChessAnalysisEngine> analysisEngine;
    std::unique_ptr<ChessSerializer> serializer;

    QTimer* matchmakingTimer;
    QTimer* gameTimer;
    QTimer* statusTimer;
    QDateTime startTime;
    QTimer* leaderboardTimer;
    
    // Maps to track clients, players, and games
    QMap<QTcpSocket*, ChessPlayer*> socketToPlayer;
    QMap<std::string, ChessPlayer*> usernamesToPlayers;
    QMap<std::string, std::unique_ptr<ChessGame>> activeGames;
    QMap<ChessPlayer*, std::string> playerToGameId;
    
    // Server statistics
    int totalGamesPlayed;
    int totalPlayersRegistered;
    int peakConcurrentPlayers;
    int totalMovesPlayed;
    
    // Process a client message
    void processClientMessage(QTcpSocket* socket, const QJsonObject& message);
    
    // Send a message to a client
    void sendMessage(QTcpSocket* socket, const QJsonObject& message);
    
    // Create a new game between two players
    std::string createGame(ChessPlayer* player1, ChessPlayer* player2, TimeControlType timeControl);
    
    // End a game
    void endGame(const std::string& gameId, GameResult result);
    
    // Process an authentication request
    void processAuthRequest(QTcpSocket* socket, const QJsonObject& data);
    
    // Process a registration request
    void processRegisterRequest(QTcpSocket* socket, const QJsonObject& data);
    
    // Process a move request
    void processMoveRequest(QTcpSocket* socket, const QJsonObject& data);
    
    // Process a matchmaking request
    void processMatchmakingRequest(QTcpSocket* socket, const QJsonObject& data);
    
    // Process a game history request
    void processGameHistoryRequest(QTcpSocket* socket, const QJsonObject& data);
    
    // Process a game analysis request
    void processGameAnalysisRequest(QTcpSocket* socket, const QJsonObject& data);
    
    // Process a resign request
    void processResignRequest(QTcpSocket* socket, const QJsonObject& data);
    
    // Process a draw offer request
    void processDrawOfferRequest(QTcpSocket* socket, const QJsonObject& data);
    
    // Process a draw response
    void processDrawResponseRequest(QTcpSocket* socket, const QJsonObject& data);
    
    // Create a bot player
    ChessPlayer* createBotPlayer(int skillLevel);
    
    // Save game history
    void saveGameHistory(const ChessGame& game);
    
    // Load all game histories
    QJsonArray loadAllGameHistories();
    
    // Update player ratings after a game
    void updatePlayerRatings(ChessGame* game);
    
    // Clean up resources for a disconnected player
    void cleanupDisconnectedPlayer(ChessPlayer* player);
    
    // Initialize the server directories
    void initializeServerDirectories();
    
    // Get the path to the game history directory
    std::string getGameHistoryPath() const;
    
    // Get the path to the player data directory
    std::string getPlayerDataPath() const;
    
    // Get the path to the logs directory
    std::string getLogsPath() const;
};

#endif // MP_CHESS_SERVER_H