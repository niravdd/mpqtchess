// src/network/ChessServer.cpp
#include "ChessProtocol.h"
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
        NetworkMessage msg;
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
/// connect(clientSocket, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::error), this, &ChessNetworkServer::handleClientError);
    connect(clientSocket, &QTcpSocket::errorOccurred,
            this, &ChessNetworkServer::handleClientError);

    // Send connection response
    NetworkMessage response;
    response.type = MessageType::CONNECT_RESPONSE;
    response.success = true;
    response.data = (slot == 0) ? "WHITE" : "BLACK";
    sendMessage(clientSocket, response);

    emit clientConnected(slot == 0 ? PieceColor::White : PieceColor::Black);

    // If both players connected, start the game
    if (clients_[0] && clients_[1]) {
        NetworkMessage startMsg;
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

    NetworkMessage msg;
    stream >> msg;
    processMessage(client, msg);
}

void ChessNetworkServer::handleClientError([[maybe_unused]] QAbstractSocket::SocketError error)
{
    QTcpSocket* client = qobject_cast<QTcpSocket*>(sender());
    if (!client) return;

    emit errorOccurred(tr("Client error: %1").arg(client->errorString()));
    cleanupClient(client);
}

void ChessNetworkServer::sendMessage(QTcpSocket* client, const NetworkMessage& msg)
{
    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    stream.setVersion(QDataStream::Qt_5_15);
    stream << msg;
    client->write(data);
}

void ChessNetworkServer::broadcastMessage(const NetworkMessage& msg)
{
    for (auto client : clients_) {
        if (client) {
            sendMessage(client, msg);
        }
    }
}

void ChessNetworkServer::processMessage(QTcpSocket* sender, const NetworkMessage& msg)
{
    // Determine player color
    PieceColor playerColor = (sender == clients_[0]) ? 
                            PieceColor::White : PieceColor::Black;

    switch (msg.type) {
        case MessageType::CONNECT_REQUEST: {
            NetworkMessage response;
            response.type = MessageType::CONNECT_RESPONSE;
            
            // Check if game is full
            if (clients_.size() >= 2) {
                response.success = false;
                response.data = "Game is full";
                sendMessage(sender, response);
                return;
            }
            
            response.success = true;
            response.data = (clients_.empty() ? "WHITE" : "BLACK");
            sendMessage(sender, response);
            
            // If we now have 2 players, start the game
            if (clients_.size() == 2) {
                NetworkMessage startMsg;
                startMsg.type = MessageType::GAME_STATE;
                startMsg.data = "started";
                broadcastMessage(startMsg);
            }
            break;
        }

        case MessageType::CONNECT_RESPONSE:
            // Server shouldn't receive this
            break;

        case MessageType::GAME_STATE:
            // Server shouldn't receive this
            break;

        case MessageType::MOVE: {
            // Validate and make move
            bool valid = game_->makeMove(msg.moveData.from, msg.moveData.to,
                                        playerColor, msg.moveData.promotionPiece);
            
            NetworkMessage response;
            response.type = MessageType::MOVE_RESPONSE;
            response.success = valid;
            response.moveData = msg.moveData;

//          response.moveData.from = msg.moveData.from;
//          response.moveData.to = msg.moveData.to;
//          response.moveData.promotionPiece = msg.moveData.promotionPiece;

            if (valid) {
                broadcastMessage(response);
                
                if (game_->isCheckmate(game_->getCurrentTurn())) {
                    NetworkMessage endMsg;
                    endMsg.type = MessageType::GAME_END;
                    endMsg.data = tr("%1 wins by checkmate!")
                        .arg(playerColor == PieceColor::White ? "White" : "Black");
                    broadcastMessage(endMsg);
                    emit gameEnded(endMsg.data);
                }
                else if (game_->isStalemate(game_->getCurrentTurn())) {
                    NetworkMessage endMsg;
                    endMsg.type = MessageType::GAME_END;
                    endMsg.data = "Game drawn by stalemate";
                    broadcastMessage(endMsg);
                    emit gameEnded(endMsg.data);
                }
            } else {
                response.data = "Invalid move";
                sendMessage(sender, response);
            }
            break;
        }

        case MessageType::MOVE_RESPONSE:
            // Server shouldn't receive this
            break;
            
        case MessageType::DRAW_OFFER: {
            // Your existing DRAW_OFFER case implementation
            QTcpSocket* opponent = (sender == clients_[0]) ? 
                                 clients_[1] : clients_[0];
            if (opponent) {
                sendMessage(opponent, msg);
            }
            break;
        }
        
        case MessageType::DRAW_RESPONSE: {
            // Your existing DRAW_RESPONSE case implementation
            QTcpSocket* opponent = (sender == clients_[0]) ? 
                                 clients_[1] : clients_[0];
            if (opponent) {
                sendMessage(opponent, msg);
            }
            
            if (msg.success) {
                NetworkMessage endMsg;
                endMsg.type = MessageType::GAME_END;
                endMsg.data = "Game drawn by agreement";
                broadcastMessage(endMsg);
                emit gameEnded(endMsg.data);
            }
            break;
        }
        
        case MessageType::RESIGN: {
            // Your existing RESIGN case implementation
            NetworkMessage endMsg;
            endMsg.type = MessageType::GAME_END;
            endMsg.data = tr("%1 resigns. %2 wins!")
                .arg(playerColor == PieceColor::White ? "White" : "Black")
                .arg(playerColor == PieceColor::White ? "Black" : "White");
            broadcastMessage(endMsg);
            emit gameEnded(endMsg.data);
            break;
        }
        
        case MessageType::CHAT: {
            // Your existing CHAT case implementation
            QTcpSocket* opponent = (sender == clients_[0]) ? 
                                 clients_[1] : clients_[0];
            if (opponent) {
                sendMessage(opponent, msg);
            }
            break;
        }

        case MessageType::ERROR:
            // Forward error messages to the appropriate client
            if (msg.data.contains("WHITE")) {
                sendMessage(clients_[0], msg);
            } else if (msg.data.contains("BLACK")) {
                sendMessage(clients_[1], msg);
            } else {
                broadcastMessage(msg);
            }
            break;

        case MessageType::KEEPALIVE:
            // Just acknowledge receipt
            sendMessage(sender, msg);
            break;

        default:
            // Handle unknown message type
            NetworkMessage errorMsg;
            errorMsg.type = MessageType::ERROR;
            errorMsg.data = "Unknown message type received";
            sendMessage(sender, errorMsg);
            break;
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
                NetworkMessage msg;
                msg.type = MessageType::GAME_END;
                msg.data = "Opponent disconnected";
                sendMessage(clients_[1-i], msg);
            }
            break;
        }
    }
    
    client->deleteLater();
}