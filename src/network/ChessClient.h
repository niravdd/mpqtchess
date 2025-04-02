// src/network/ChessClient.h
#pragma once

#include "../QtCompat.h"
#include <QObject>
#include <QTcpSocket>
#include <QHostAddress>
#include <memory>
#include "../core/ChessGame.h"
#include "ChessProtocol.h"

class ChessClient : public QObject {
    Q_OBJECT

public:
    explicit ChessClient(QObject* parent = nullptr);
    ~ChessClient();

    // Connection management
    bool connectToServer(const QString& address, quint16 port);
    void disconnect();
    bool isConnected() const;
    PieceColor getPlayerColor() const { return playerColor_; }

    // Game actions
    void sendMove(const Position& from, const Position& to, PieceType promotionPiece = PieceType::None);
    void offerDraw();
    void respondToDraw(bool accept);
    void resign();
    void sendChatMessage(const QString& message);

signals:
    // Connection signals
    void connected(PieceColor playerColor);
    void connectionError(const QString& error);
    void disconnected();
    
    // Game state signals
    void gameStarted();
    void gameEnded(const QString& reason);
    void turnChanged(PieceColor color);
    
    // Move signals
    void moveReceived(const Position& from, const Position& to, PieceType promotionPiece);
    void moveValidated(bool valid, const QString& error);
    void moveMade(const Position& from, const Position& to);
    
    // Game event signals
    void checkOccurred();
    void checkmateOccurred(PieceColor winner);
    void stalemateOccurred();
    void drawOccurred(const QString& reason);
    
    // Player interaction signals
    void drawOffered();
    void drawResponseReceived(bool accepted);
    void playerResigned(PieceColor color);
    void chatMessageReceived(const QString& message);

private slots:
    void handleConnected();
    void handleDisconnected();
    void handleError(QAbstractSocket::SocketError socketError);
    void handleReadyRead();

private:
    void sendMessage(const NetworkMessage& msg);
    void processMessage(const NetworkMessage& msg);
    void processGameStateMessage(const NetworkMessage& msg);
    void processMoveMessage(const NetworkMessage& msg);
    void processPlayerActionMessage(const NetworkMessage& msg);
    void processConnectResponse(const NetworkMessage& msg);
    
    QTcpSocket socket_;
    PieceColor playerColor_;
    bool gameInProgress_;
    QByteArray receivedData_;

    // Network message handling
    void appendToBuffer(const QByteArray& data);
    bool tryProcessNextMessage();
    NetworkMessage parseMessage(const QByteArray& data);
    
    // Message validation
    bool validateMove(const Position& from, const Position& to) const;
    bool isValidGameState(const QString& state) const;
    
    // Error handling
    void handleNetworkError(const QString& error);
    void handleProtocolError(const QString& error);
    
    // Game state tracking
    std::unique_ptr<ChessGame> localGame_;  // For move validation
    bool myTurn_;
    bool drawPending_;
};

// Inline utility functions
inline bool ChessClient::isConnected() const {
    return socket_.state() == ::QAbstractSocket::ConnectedState;
}

// Network message size constants
namespace {
    constexpr int MAX_MESSAGE_SIZE = 1024;    // Maximum message size in bytes
    constexpr int HEADER_SIZE = sizeof(quint32);  // Size of message header
    constexpr int KEEPALIVE_INTERVAL = 30000;     // Keepalive interval in ms
}

// Message validation constants
namespace {
    const QStringList VALID_GAME_STATES = {
        "waiting", "started", "ended", "draw", "resigned"
    };
}

// Socket timeouts
namespace {
    constexpr int CONNECTION_TIMEOUT = 5000;      // Connection timeout in ms
    constexpr int RESPONSE_TIMEOUT = 10000;       // Response timeout in ms
}

// Error messages
namespace {
    const QString ERR_INVALID_MOVE = "Invalid move";
    const QString ERR_NOT_YOUR_TURN = "Not your turn";
    const QString ERR_GAME_NOT_STARTED = "Game not started";
    const QString ERR_CONNECTION_FAILED = "Failed to connect to server";
    const QString ERR_PROTOCOL_ERROR = "Protocol error";
}