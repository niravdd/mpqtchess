// src/network/ChessServer.h
#pragma once
#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <array>
#include "ChessProtocol.h"
#include "../core/ChessGame.h"

class ChessNetworkServer : public QObject {
    Q_OBJECT

public:
    explicit ChessNetworkServer(QObject* parent = nullptr);
    
    bool start(quint16 port = 12345);
    void stop();

signals:
    void clientConnected(PieceColor color);
    void clientDisconnected(PieceColor color);
    void gameStarted();
    void gameEnded(const QString& reason);
    void errorOccurred(const QString& error);

private slots:
    void handleNewConnection();
    void handleClientDisconnected();
    void handleClientData();
    void handleClientError(QAbstractSocket::SocketError error);

private:
QTcpServer server_;
    std::array<QTcpSocket*, 2> clients_;
    std::unique_ptr<ChessGame> game_;
    
    void sendMessage(QTcpSocket* client, const NetworkMessage& msg);
    void broadcastMessage(const NetworkMessage& msg);
    void processMessage(QTcpSocket* sender, const NetworkMessage& msg);
    void cleanupClient(QTcpSocket* client);
};