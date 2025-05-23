// NetworkManager.h
#pragma once

#include <QObject>
#include <QTcpSocket>
#include <QHostAddress>
#include <QString>
#include <QByteArray>
#include <QQueue>
#include <QMutex>
#include <QTimer>

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
    QByteArray payload;
};

class NetworkManager : public QObject {
    Q_OBJECT

public:
    static NetworkManager& getInstance();

    bool connectToServer(const QString& host, quint16 port);
    void disconnectFromServer();
    bool isConnected() const;
    
    bool sendMessage(const Message& message);

signals:
    void connected();
    void disconnected();
    void errorOccurred(const QString& errorMessage);
    void messageReceived(const Message& message);

public slots:
    void sendPing();

private slots:
    void onSocketConnected();
    void onSocketDisconnected();
    void onSocketError(QAbstractSocket::SocketError socketError);
    void onReadyRead();
    void processMessages();

private:
    NetworkManager(QObject* parent = nullptr);
    ~NetworkManager();
    
    QTcpSocket* socket;
    QQueue<Message> incomingMessages;
    QMutex messageMutex;
    QTimer pingTimer;
    
    bool sendRawMessage(const QByteArray& data);
    Message parseMessage(const QByteArray& data);
};