// src/network/ChessServer.cpp
#include "ChessServer.h"
#include <QDataStream>

ChessNetworkServer::ChessNetworkServer(QObject* parent)
    : QObject(parent)
    , clients_({nullptr, nullptr})
    , game_(std::make_unique<ChessGame>())
{
    connect(&server_, &QTcpServer::newConnection,
            this, &ChessNetworkServer::handleNewConnection);
}

bool ChessNetworkServer::start(quint16 port)
{
    if (!server_.listen(QHostAddress::Any, port)) {
        emit errorOccurred(tr("Failed to start server: %1")
                          .arg(server_.errorString()));
        return false;
    }
    return true;
}

void ChessNetworkServer::stop()
{
    for (auto client : clients_) {
        if (client) {
            client->disconnectFromHost();
        }
    }
    server_.close();
}

void ChessNetworkServer::handleNewConnection()
{
    QTcpSocket* clientSocket = server_.nextPendingConnection();
    if (!clientSocket) return;

    // Find available slot
    int slot = -1;
    for (int i = 0; i < 2; ++i) {
        if (!clients_[i]) {
            slot = i;
            break;
        }
    }

    if (slot == -1) {
        // No slots available
        ChessMessage msg;
        msg.type = MessageType::ERROR;
        msg.data = "Game is full";
        sendMessage(clientSocket, msg);
        clientSocket->disconnectFromHost();
        return;
    }

    clients_[slot] = clientSocket;
    
    connect(clientSocket, &QTcpSocket::disconnected,
            this, &ChessNetworkServer::handleClientDisconnected);
    connect(clientSocket, &QTcpSocket::readyRead,
            this, &ChessNetworkServer::handleClientData);
    connect(clientSocket, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::error),
            this, &ChessNetworkServer::handleClientError);

    // Send connection response
    ChessMessage response;
    response.type = MessageType::CONNECT_RESPONSE;
    response.success = true;
    response.data = (slot == 0) ? "WHITE" : "BLACK";
    sendMessage(clientSocket, response);

    emit clientConnected(slot == 0 ? PieceColor::White : PieceColor::Black);

    // If both players connected, start the game
    if (clients_[0] && clients_[1]) {
        ChessMessage startMsg;
        startMsg.type = MessageType::GAME_END;
        startMsg.data = "Game started";
        broadcastMessage(startMsg);
        emit gameStarted();
    }
}

void ChessNetworkServer::handleClientDisconnected()
{
    QTcpSocket* client = qobject_cast<QTcpSocket*>(sender());
    if (!client) return;

    cleanupClient(client);
}

void ChessNetworkServer::handleClientData()
{
    QTcpSocket* client = qobject_cast<QTcpSocket*>(sender());
    if (!client) return;

    QByteArray data = client->readAll();
    QDataStream stream(data);
    stream.setVersion(QDataStream::Qt_5_15);

    ChessMessage msg;
    stream >> msg;
    processMessage(client, msg);
}

void ChessNetworkServer::handleClientError(QAbstractSocket::SocketError error)
{
    QTcpSocket* client = qobject_cast<QTcpSocket*>(sender());
    if (!client) return;

    emit errorOccurred(tr("Client error: %1").arg(client->errorString()));
    cleanupClient(client);
}

void ChessNetworkServer::sendMessage(QTcpSocket* client, const ChessMessage& msg)
{
    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    stream.setVersion(QDataStream::Qt_5_15);
    stream << msg;
    client->write(data);
}

void ChessNetworkServer::broadcastMessage(const ChessMessage& msg)
{
    for (auto client : clients_) {
        if (client) {
            sendMessage(client, msg);
        }
    }
}

void ChessNetworkServer::processMessage(QTcpSocket* sender, const ChessMessage& msg)
{
    // Determine player color
    PieceColor playerColor = (sender == clients_[0]) ? 
                            PieceColor::White : PieceColor::Black;

    switch (msg.type) {
        case MessageType::MOVE: {
            // Validate and make move
            bool valid = game_->makeMove(msg.from, msg.to, 
                                       playerColor, msg.promotionPiece);
            
            ChessMessage response;
            response.type = MessageType::MOVE_RESPONSE;
            response.success = valid;
            response.from = msg.from;
            response.to = msg.to;
            response.promotionPiece = msg.promotionPiece;
            
            if (valid) {
                // Broadcast valid move to both players
                broadcastMessage(response);
                
                // Check game state
                if (game_->isCheckmate(game_->getCurrentTurn())) {
                    ChessMessage endMsg;
                    endMsg.type = MessageType::GAME_END;
                    endMsg.data = tr("%1 wins by checkmate!")
                        .arg(playerColor == PieceColor::White ? "White" : "Black");
                    broadcastMessage(endMsg);
                    emit gameEnded(endMsg.data);
                }
                else if (game_->isStalemate(game_->getCurrentTurn())) {
                    ChessMessage endMsg;
                    endMsg.type = MessageType::GAME_END;
                    endMsg.data = "Game drawn by stalemate";
                    broadcastMessage(endMsg);
                    emit gameEnded(endMsg.data);
                }
            } else {
                // Send error response only to sender
                response.data = "Invalid move";
                sendMessage(sender, response);
            }
            break;
        }
        
        case MessageType::DRAW_OFFER: {
            // Forward draw offer to opponent
            QTcpSocket* opponent = (sender == clients_[0]) ? 
                                 clients_[1] : clients_[0];
            if (opponent) {
                sendMessage(opponent, msg);
            }
            break;
        }
        
        case MessageType::DRAW_RESPONSE: {
            // Forward draw response to opponent
            QTcpSocket* opponent = (sender == clients_[0]) ? 
                                 clients_[1] : clients_[0];
            if (opponent) {
                sendMessage(opponent, msg);
            }
            
            if (msg.success) {
                // Game ended in draw
                ChessMessage endMsg;
                endMsg.type = MessageType::GAME_END;
                endMsg.data = "Game drawn by agreement";
                broadcastMessage(endMsg);
                emit gameEnded(endMsg.data);
            }
            break;
        }
        
        case MessageType::RESIGN: {
            ChessMessage endMsg;
            endMsg.type = MessageType::GAME_END;
            endMsg.data = tr("%1 resigns. %2 wins!")
                .arg(playerColor == PieceColor::White ? "White" : "Black")
                .arg(playerColor == PieceColor::White ? "Black" : "White");
            broadcastMessage(endMsg);
            emit gameEnded(endMsg.data);
            break;
        }
        
        case MessageType::CHAT_MESSAGE: {
            // Forward chat message to opponent
            QTcpSocket* opponent = (sender == clients_[0]) ? 
                                 clients_[1] : clients_[0];
            if (opponent) {
                sendMessage(opponent, msg);
            }
            break;
        }
    }
}

void ChessNetworkServer::cleanupClient(QTcpSocket* client)
{
    for (int i = 0; i < 2; ++i) {
        if (clients_[i] == client) {
            clients_[i] = nullptr;
            emit clientDisconnected(i == 0 ? PieceColor::White : PieceColor::Black);
            
            // Notify remaining client
            if (clients_[1-i]) {
                ChessMessage msg;
                msg.type = MessageType::GAME_END;
                msg.data = "Opponent disconnected";
                sendMessage(clients_[1-i], msg);
            }
            break;
        }
    }
    
    client->deleteLater();
}