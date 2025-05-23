// MPChessServer.h
#ifndef MP_CHESS_SERVER_H
#define MP_CHESS_SERVER_H

#include <array>
#include <chrono>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <random>
#include <set>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <variant>
#include <vector>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    #define SOCKET_ERROR_CODE WSAGetLastError()
    using socket_t = SOCKET;
    #define CLOSE_SOCKET(s) closesocket(s)
    #define SOCKET_CLEANUP() WSACleanup()
    #define SOCKET_INIT() { WSADATA wsaData; WSAStartup(MAKEWORD(2,2), &wsaData); }
    #define INVALID_SOCKET_VALUE INVALID_SOCKET
#else
    #include <arpa/inet.h>
    #include <fcntl.h>
    #include <netdb.h>
    #include <netinet/in.h>
    #include <sys/socket.h>
    #include <sys/types.h>
    #include <unistd.h>
    #include <errno.h>
    #define SOCKET_ERROR_CODE errno
    using socket_t = int;
    #define CLOSE_SOCKET(s) close(s)
    #define SOCKET_CLEANUP() 
    #define SOCKET_INIT()
    #define INVALID_SOCKET_VALUE -1
#endif

namespace chess {

// Forward declarations
class ChessGame;
class ChessServer;
class NetworkManager;
class ChessBot;

// Logging utility
class Logger {
public:
    enum class Level { DEBUG, INFO, WARNING, ERROR, FATAL };
    
    static Logger& getInstance() {
        static Logger instance;
        return instance;
    }
    
    void setLogFile(const std::string& filename);
    void log(Level level, const std::string& message);
    void debug(const std::string& message);
    void info(const std::string& message);
    void warning(const std::string& message);
    void error(const std::string& message);
    void fatal(const std::string& message);
    
private:
    Logger();
    ~Logger();
    
    std::mutex logMutex_;
    std::ofstream logFile_;
    bool consoleOutput_ = true;
    
    std::string getLevelString(Level level);
    std::string getTimestamp();
};

// Chess pieces and board representation
enum class PieceType { NONE, PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING };
enum class PieceColor { WHITE, BLACK };

struct ChessPiece {
    PieceType type = PieceType::NONE;
    PieceColor color = PieceColor::WHITE;
    bool hasMoved = false;
    
    ChessPiece() = default;
    ChessPiece(PieceType t, PieceColor c) : type(t), color(c) {}
    
    bool operator==(const ChessPiece& other) const {
        return type == other.type && color == other.color && hasMoved == other.hasMoved;
    }
    
    bool operator!=(const ChessPiece& other) const {
        return !(*this == other);
    }
    
    char toChar() const;
    static ChessPiece fromChar(char c);
};

struct Position {
    int row;    // 0-7, 0 is the white's first rank
    int col;    // 0-7, 0 is the a-file
    
    Position() : row(0), col(0) {}
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
    
    std::string toAlgebraic() const;
    static Position fromAlgebraic(const std::string& algebraic);
};

struct Move {
    Position from;
    Position to;
    PieceType promotionPiece = PieceType::NONE;
    
    Move() = default;
    Move(const Position& f, const Position& t) : from(f), to(t) {}
    Move(const Position& f, const Position& t, PieceType promotion)
        : from(f), to(t), promotionPiece(promotion) {}
    
    bool operator==(const Move& other) const {
        return from == other.from && to == other.to && promotionPiece == other.promotionPiece;
    }
    
    bool operator!=(const Move& other) const {
        return !(*this == other);
    }
    
    std::string toAlgebraic() const;
    static Move fromAlgebraic(const std::string& algebraic);
    static Move fromUCI(const std::string& uci);
    std::string toUCI() const;
};

struct MoveInfo {
    Move move;
    PieceType capturedPiece = PieceType::NONE;
    bool isEnPassant = false;
    bool isCastle = false;
    bool isPromotion = false;
    bool isCheck = false;
    bool isCheckmate = false;
    bool isStalemate = false;
    Position capturedPiecePos;  // Used for en passant
    Position rookFromPos;       // Used for castling
    Position rookToPos;         // Used for castling
    
    std::string toNotation() const;
};

struct GameState {
    using Board = std::array<std::array<ChessPiece, 8>, 8>;
    
    Board board;
    PieceColor currentTurn = PieceColor::WHITE;
    bool whiteCanCastleKingside = true;
    bool whiteCanCastleQueenside = true;
    bool blackCanCastleKingside = true;
    bool blackCanCastleQueenside = true;
    std::optional<Position> enPassantTarget;
    int halfMoveClock = 0;  // For 50-move rule
    int fullMoveNumber = 1;
    std::vector<MoveInfo> moveHistory;
    
    GameState();
    
    static GameState createStandardBoard();
    static GameState fromFEN(const std::string& fen);
    std::string toFEN() const;
    
    bool operator==(const GameState& other) const;
    bool operator!=(const GameState& other) const;
};

enum class GameTimeControlType {
    BULLET,    // 1-2 minutes per side
    BLITZ,     // 3-5 minutes per side
    RAPID,     // 10-15 minutes per side
    CLASSICAL, // 60+ minutes per side
    CORRESPONDENCE // Days per move
};

struct GameTimeControl {
    GameTimeControlType type;
    std::chrono::milliseconds initialTime;
    std::chrono::milliseconds increment;
    
    static GameTimeControl createBullet() {
        return {GameTimeControlType::BULLET, std::chrono::minutes(2), std::chrono::seconds(1)};
    }
    
    static GameTimeControl createBlitz() {
        return {GameTimeControlType::BLITZ, std::chrono::minutes(5), std::chrono::seconds(2)};
    }
    
    static GameTimeControl createRapid() {
        return {GameTimeControlType::RAPID, std::chrono::minutes(15), std::chrono::seconds(10)};
    }
    
    static GameTimeControl createClassical() {
        return {GameTimeControlType::CLASSICAL, std::chrono::minutes(90), std::chrono::seconds(30)};
    }
    
    static GameTimeControl createCorrespondence(int daysPerMove) {
        return {GameTimeControlType::CORRESPONDENCE, std::chrono::hours(24) * daysPerMove, std::chrono::seconds(0)};
    }
};

// Possible states a game can be in
enum class GameStatus {
    WAITING_FOR_PLAYERS,
    PLAYING,
    WHITE_WON,
    BLACK_WON,
    DRAW,
    ABANDONED
};

struct Player {
    socket_t socket = INVALID_SOCKET_VALUE;
    std::string name;
    PieceColor color;
    bool isBot = false;
    bool connected = false;
    std::chrono::milliseconds remainingTime;
    std::chrono::time_point<std::chrono::steady_clock> moveStartTime;
    
    Player() = default;
};

// Protocol messages
enum class MessageType {
    CONNECT,
    GAME_START,
    MOVE,
    MOVE_RESULT,
    POSSIBLE_MOVES,
    GAME_END,
    CHAT,
    ERROR,
    TIME_UPDATE,
    REQUEST_DRAW,
    RESIGN,
    PING,
    PONG,
    SAVE_GAME,
    LOAD_GAME,
    LOGIN,
    REGISTER,
    MATCHMAKING_REQUEST,
    MATCHMAKING_STATUS,
    GAME_ANALYSIS,
    PLAYER_STATS,
    LEADERBOARD_REQUEST,
    LEADERBOARD_RESPONSE,
    MOVE_RECOMMENDATIONS
};

struct Message {
    MessageType type;
    std::string payload;
    socket_t senderSocket = INVALID_SOCKET_VALUE;
};

// User account and authentication
struct UserAccount {
    std::string username;
    std::string passwordHash;
    int rating;
    int gamesPlayed;
    int wins;
    int losses;
    int draws;
    std::string preferredTimeControl;
    std::chrono::system_clock::time_point lastLogin;
    std::chrono::system_clock::time_point registrationDate;
    std::vector<uint32_t> savedGameIds;
    
    UserAccount() : rating(1200), gamesPlayed(0), wins(0), losses(0), draws(0) {}
    
    std::string serialize() const;
    static UserAccount deserialize(const std::string& data);
};

class UserManager {
public:
    static UserManager& getInstance() {
        static UserManager instance;
        return instance;
    }
    
    bool initialize(const std::string& userDbFile = "chess_users.db");
    void shutdown();
    
    bool registerUser(const std::string& username, const std::string& password);
    bool authenticateUser(const std::string& username, const std::string& password);
    bool updateUser(const UserAccount& user);
    std::optional<UserAccount> getUser(const std::string& username);
    
    bool addSavedGameToUser(const std::string& username, uint32_t gameId);
    std::vector<uint32_t> getUserSavedGames(const std::string& username);
    
    // Rating system functions
    int calculateNewRating(int currentRating, int opponentRating, double score);
    void updateRatings(const std::string& winner, const std::string& loser, bool isDraw = false);

    // Leaderboard functionality
    std::vector<UserAccount> getTopPlayers(int count = 50);

private:
    UserManager() = default;
    ~UserManager();
    
    std::string hashPassword(const std::string& password);
    bool verifyPassword(const std::string& password, const std::string& hash);
    
    std::unordered_map<std::string, UserAccount> users_;
    std::mutex usersMutex_;
    std::string databaseFile_;
    bool loadUsers();
    bool saveUsers();
};

// Matchmaking system
enum class MatchmakingStatus {
    IDLE,
    SEARCHING,
    MATCHED
};

struct MatchmakingRequest {
    std::string username;
    socket_t socket;
    int rating;
    std::string preferredTimeControl;
    std::chrono::steady_clock::time_point requestTime;
};

class MatchmakingSystem {
public:
    static MatchmakingSystem& getInstance() {
        static MatchmakingSystem instance;
        return instance;
    }
    
    void addRequest(const MatchmakingRequest& request);
    void removeRequest(socket_t socket);
    void processMatchmaking(ChessServer* server);
    bool isUserInMatchmaking(const std::string& username);
    
private:
    MatchmakingSystem() = default;
    
    std::vector<MatchmakingRequest> requests_;
    std::mutex requestsMutex_;
};

// Game analysis features
struct GameAnalysis {
    std::vector<std::string> annotations;
    int whiteAccuracy;
    int blackAccuracy;
    std::vector<int> evaluations;  // Centipawn evaluation after each move
    
    std::string serialize() const;
    static GameAnalysis deserialize(const std::string& data);
};

// Class to handle chess game logic
class ChessGame {
public:
    ChessGame(uint32_t gameId, const GameTimeControl& timeControl, ChessServer* server);
    ~ChessGame();
    
    uint32_t getId() const { return gameId_; }
    GameStatus getStatus() const { return status_; }
    const GameState& getState() const { return state_; }
    
    bool addPlayer(const Player& player);
    bool addBotPlayer(PieceColor color);
    void start();
    void stop();
    
    MoveInfo processMove(socket_t playerSocket, const Move& move);
    std::vector<Move> getPossibleMoves(const Position& position) const;
    std::vector<Move> getPossibleMovesForPlayer(PieceColor color) const;
    
    void updateTimers();
    void playerDisconnected(socket_t playerSocket);
    bool isPlayersTurn(socket_t playerSocket) const;
    
    bool isCheckmate() const;
    bool isStalemate() const;
    bool isCheck() const;
    bool isInsufficientMaterial() const;
    bool isThreefoldRepetition() const;
    bool isFiftyMoveRule() const;
    
    bool requestDraw(socket_t playerSocket);
    void resignGame(socket_t playerSocket);
    
    std::string saveGame() const;
    bool loadGame(const std::string& savedGame);
    
    std::string getAsciiBoard() const;

    // Authentication and user features
    bool setPlayerFromUser(socket_t socket, const std::string& username);
    const std::string& getWhitePlayerName() const { return whitePlayerName_; }
    const std::string& getBlackPlayerName() const { return blackPlayerName_; }

    // Move recommendations
    std::vector<std::pair<Move, double>> getRecommendedMoves(PieceColor color, int maxMoves = 3);
    
    // Game analysis features
    void analyzeGame();
    const GameAnalysis& getGameAnalysis() const { return analysis_; }
    void annotateMove(const std::string& annotation);
    
    // Enhanced bot features
    void setBotDifficulty(int difficulty);
    int getBotEloRating() const;

private:
    uint32_t gameId_;
    GameStatus status_ = GameStatus::WAITING_FOR_PLAYERS;
    GameState state_;
    GameTimeControl timeControl_;
    
    Player whitePlayer_;
    Player blackPlayer_;
    std::unique_ptr<ChessBot> botPlayer_;
    
    ChessServer* server_;
    std::thread gameThread_;
    std::mutex gameMutex_;
    bool gameRunning_ = false;
    bool drawRequested_ = false;
    socket_t drawRequestedBy_ = INVALID_SOCKET_VALUE;

    std::string whitePlayerName_;
    std::string blackPlayerName_;
    GameAnalysis analysis_;
    
    // Flags for user authentication
    bool whiteIsAuthenticated_ = false;
    bool blackIsAuthenticated_ = false;
    
    std::unordered_map<std::string, int> positionCount_;  // For threefold repetition
    
    bool isValidMove(const Move& move, PieceColor playerColor) const;
    MoveInfo makeMove(const Move& move);
    void unmakeMove(const MoveInfo& moveInfo);
    
    bool isSquareAttacked(const Position& pos, PieceColor attackingColor) const;
    Position findKing(PieceColor color) const;
    
    bool isCastlingMove(const Move& move) const;
    bool isEnPassantMove(const Move& move) const;
    bool isPawnPromotion(const Move& move) const;
    
    std::vector<Move> generatePawnMoves(const Position& pos) const;
    std::vector<Move> generateKnightMoves(const Position& pos) const;
    std::vector<Move> generateBishopMoves(const Position& pos) const;
    std::vector<Move> generateRookMoves(const Position& pos) const;
    std::vector<Move> generateQueenMoves(const Position& pos) const;
    std::vector<Move> generateKingMoves(const Position& pos) const;
    
    void sendGameState();
    void sendTimeUpdate();
    void botMove();
    
    void gameLoop();
};

// Class for the chess engine AI opponent
class ChessBot {
public:
    ChessBot(PieceColor color, int difficulty = 3);
    
    Move getNextMove(const GameState& state);
    void setDifficulty(int difficulty);
    PieceColor getColor() const { return color_; }
    
private:
    PieceColor color_;
    int difficulty_; // 1-5, higher is more difficult
    
    int evaluatePosition(const GameState& state) const;
    Move minimaxRoot(const GameState& state, int depth);
    int minimax(GameState state, int depth, int alpha, int beta, bool isMaximizing);
};

// Class to manage client connections and game instances
class ChessServer {
public:
    ChessServer(uint16_t port = 5000);
    ~ChessServer();
    
    bool start();
    void stop();
    
    ChessGame* createGame(const GameTimeControl& timeControl);
    void removeGame(uint32_t gameId);
    ChessGame* findGame(uint32_t gameId);
    ChessGame* findGameByPlayerSocket(socket_t socket);
    
    void broadcastToGame(uint32_t gameId, const Message& message);
    void sendToPlayer(socket_t playerSocket, const Message& message);
    
    uint16_t getPort() const { return port_; }
    bool isRunning() const { return running_; }

private:
    uint16_t port_;
    bool running_ = false;
    socket_t serverSocket_ = INVALID_SOCKET_VALUE;
    std::thread acceptorThread_;
    std::vector<std::thread> workerThreads_;
    
    std::mutex gamesMutex_;
    std::unordered_map<uint32_t, std::unique_ptr<ChessGame>> games_;
    
    std::mutex clientsMutex_;
    std::set<socket_t> clients_;
    std::queue<Message> messageQueue_;
    std::mutex queueMutex_;
    std::condition_variable queueCondition_;
    
    std::atomic<uint32_t> nextGameId_ = 1;
    
    void acceptorLoop();
    void workerLoop();
    void handleMessage(const Message& message);
    
    void handleClientConnect(socket_t clientSocket, const std::string& payload);
    void handleClientDisconnect(socket_t clientSocket);
    void handleMoveRequest(socket_t clientSocket, const std::string& payload);
    void handleDrawRequest(socket_t clientSocket);
    void handleResignRequest(socket_t clientSocket);
    void handleSaveGame(socket_t clientSocket, const std::string& payload);
    void handleLoadGame(socket_t clientSocket, const std::string& payload);
    
    void clientReader(socket_t clientSocket);
    
    uint32_t generateGameId();
    
    friend class NetworkManager;
};

// Class to handle serialization and deserialization
class Serializer {
public:
    static std::string serializeGameState(const GameState& state);
    static GameState deserializeGameState(const std::string& data);
    
    static std::string serializeMessage(const Message& message);
    static Message deserializeMessage(const std::string& data);
    
    static std::string serializeMove(const Move& move);
    static Move deserializeMove(const std::string& data);
    
    static std::string serializeMoveInfo(const MoveInfo& moveInfo);
    static MoveInfo deserializeMoveInfo(const std::string& data);
    
    static std::string serializeGame(const ChessGame& game);
    static std::unique_ptr<ChessGame> deserializeGame(const std::string& data, ChessServer* server);
};

// Class to handle network communications
class NetworkManager {
public:
    static NetworkManager& getInstance() {
        static NetworkManager instance;
        return instance;
    }
    
    bool initialize();
    void cleanup();
    
    bool sendMessage(socket_t socket, const Message& message);
    std::optional<Message> receiveMessage(socket_t socket);
    
    socket_t createServerSocket(uint16_t port);
    socket_t acceptClient(socket_t serverSocket);
    void closeSocket(socket_t socket);
    
    std::string getErrorString();
    
private:
    NetworkManager() = default;
};

} // namespace chess

#endif // MP_CHESS_SERVER_H