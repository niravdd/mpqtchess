// src/network/ChessServer.cpp
#include "ChessProtocol.h"
#include "ChessServer.h"
#include <QDataStream>
#include <QDateTime>

ChessNetworkServer::ChessNetworkServer(QObject* parent)
    : QObject(parent)
    , clients_({nullptr, nullptr})
    , game_(std::make_unique<ChessGame>())
{
    connect(&server_, &::QTcpServer::newConnection,
            this, &ChessNetworkServer::handleNewConnection);
}

void ChessNetworkServer::logMessage(const QString& message)
{
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    qDebug() << "[ChessServer]" << timestamp << ":" << message;
}

bool ChessNetworkServer::start(quint16 port)
{
    if (!server_.listen(::QHostAddress::Any, port)) {
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
    logMessage("New client connection received.");
    
    QTcpSocket* clientSocket = server_.nextPendingConnection();
    if (!clientSocket) {
        logMessage("Failed to get the pending connection.");
        return;
    }

    // Find available slot
    int slot = -1;
    for (int i = 0; i < 2; ++i) {
        if (!clients_[i]) {
            slot = i;
            break;
        }
    }

    if (slot == -1) {
        logMessage("No available slots for the new client, sending error response.");
        
        NetworkMessage msg;
        msg.type = MessageType::ERROR;
        msg.data = "Game is full";
        sendMessage(clientSocket, msg);
        clientSocket->disconnectFromHost();
        return;
    }

    clients_[slot] = clientSocket;
    
    logMessage(QString("New client connected, assigned to slot %1.").arg(slot));
    
    connect(clientSocket, &::QTcpSocket::disconnected,
            this, &ChessNetworkServer::handleClientDisconnected);
    connect(clientSocket, &::QTcpSocket::readyRead,
            this, &ChessNetworkServer::handleClientData);
    connect(clientSocket, &::QTcpSocket::errorOccurred,
            this, &ChessNetworkServer::handleClientError);

    // Send connection response
    NetworkMessage response;
    response.type = MessageType::CONNECT_RESPONSE;
    response.success = true;
    response.data = (slot == 0) ? "WHITE" : "BLACK";
    sendMessage(clientSocket, response);
    logMessage(QString("Sent connection response to the new client, assigned color: %1.").arg(response.data));

    emit clientConnected(slot == 0 ? PieceColor::White : PieceColor::Black);

    // If both players connected, start the game
    if (clients_[0] && clients_[1]) {
        logMessage("Both players connected, starting the game.");
        
        NetworkMessage startMsg;
        startMsg.type = MessageType::GAME_STATE;
        startMsg.data = "started";
        broadcastMessage(startMsg);
        emit gameStarted();
    }
}

void ChessNetworkServer::handleClientDisconnected() {
    QTcpSocket* client = qobject_cast<QTcpSocket*>(sender());
    if (!client) {
        logMessage("handleClientDisconnected: Failed to cast the sender to QTcpSocket*.");
        return;
    }

    logMessage(QString("Client disconnected: %1").arg(client->peerAddress().toString()));
    cleanupClient(client);
}

void ChessNetworkServer::handleClientData() {
    QTcpSocket* client = qobject_cast<QTcpSocket*>(sender());
    if (!client) {
        logMessage("handleClientData: Failed to cast the sender to QTcpSocket*.");
        return;
    }

    QByteArray data = client->readAll();
    logMessage(QString("Received data from client: %1").arg(client->peerAddress().toString()));
    logMessage(QString("Data received: %1").arg(QString(data)));

    QDataStream stream(data);
    stream.setVersion(::QDataStream::Qt_5_15);

    NetworkMessage msg;
    stream >> msg;
    processMessage(client, msg);
}

void ChessNetworkServer::handleClientError(QAbstractSocket::SocketError error)
{
    QTcpSocket* client = qobject_cast<QTcpSocket*>(sender());
    if (!client) {
        logMessage("handleClientError: Failed to cast the sender to QTcpSocket*.");
        return;
    }

    logMessage(QString("Client error: %1, Error code: %2").arg(client->errorString()).arg(error));
    emit errorOccurred(tr("Client error: %1").arg(client->errorString()));
    cleanupClient(client);
}

void ChessNetworkServer::sendMessage(QTcpSocket* client, const NetworkMessage& msg)
{
    QByteArray data;
    QDataStream stream(&data, ::QIODevice::WriteOnly);
    stream.setVersion(::QDataStream::Qt_5_15);
    stream << msg;

    logMessage(QString("Sending data to client: %1").arg(client->peerAddress().toString()));
    logMessage(QString("Data sent: %1").arg(QString(data)));

    client->write(data);
}

void ChessNetworkServer::broadcastMessage(const NetworkMessage& msg) {
    logMessage(QString("Broadcasting message to all clients. Message type: %1").arg(static_cast<int>(msg.type)));

    for (auto client : clients_) {
        if (client) {
            sendMessage(client, msg);
        }
    }
}

void ChessNetworkServer::processMessage(QTcpSocket* sender, const NetworkMessage& msg)
{
    // Determine player color
    // amazonq-ignore-next-line
    PieceColor playerColor = (sender == clients_[0]) ? 
                            PieceColor::White : PieceColor::Black;

    logMessage(QString("Processing message from client %1. Message type: %2")
                            .arg(sender->peerAddress().toString())
                            .arg(static_cast<int>(msg.type)));

    switch (msg.type) {
        case MessageType::CONNECT_REQUEST: {
            logMessage(QString("In MessageType::CONNECT_REQUEST, sender: %1").arg(sender->peerAddress().toString()));
            NetworkMessage response;
            response.type = MessageType::CONNECT_RESPONSE;
            
            // Check if game is full
            if (clients_.size() >= 2) {
                response.success = false;
                response.data = "Game is full";
                logMessage(QString("In MessageType::CONNECT_REQUEST, Game is full, returning!"));
                sendMessage(sender, response);
                return;
            }
            
            response.success = true;
            response.data = (clients_.empty() ? "WHITE" : "BLACK");
            logMessage(QString("In MessageType::CONNECT_REQUEST, sender: %1, response.data: %2")
                            .arg(sender->peerAddress().toString())
                            .arg(response.data));
            sendMessage(sender, response);
            
            // If we now have 2 players, start the game
            if (clients_.size() == 2) {
                NetworkMessage startMsg;
                startMsg.type = MessageType::GAME_STATE;
                startMsg.data = "started";
                logMessage(QString("In MessageType::CONNECT_REQUEST, sender: %1, broadcastMessage.type: %2, broadcastMessage.data: %3")
                            .arg(sender->peerAddress().toString())
                            .arg(messageTypeToString(startMsg.type))
                            .arg(startMsg.data));
                broadcastMessage(startMsg);
            }
            break;
        }

        case MessageType::CONNECT_RESPONSE: {
            // Server shouldn't receive this
            logMessage(QString("In MessageType::CONNECT_RESPONSE, UNEXPECTED Message - sender: %1").arg(sender->peerAddress().toString()));
            break;
        }

        case MessageType::GAME_STATE: {
            // Server shouldn't receive this
            logMessage(QString("In MessageType::GAME_STATE, UNEXPECTED Message - sender: %1").arg(sender->peerAddress().toString()));
            break;
        }

        case MessageType::MOVE: {
            // Validate and make move
            logMessage(QString("In MessageType::MOVE, sender: %1, msg.moveData.from: %2, msg.moveData.to: %3, msg.moveData.promotionPiece: %4")
                            .arg(sender->peerAddress().toString())
                            .arg(QString("%1,%2").arg(msg.moveData.from.row).arg(msg.moveData.from.col))
                            .arg(QString("%1,%2").arg(msg.moveData.to.row).arg(msg.moveData.to.col))
                            .arg(pieceTypeToString(msg.moveData.promotionPiece)));
           
            bool valid = game_->makeMove(msg.moveData.from, msg.moveData.to,
                                        playerColor, msg.moveData.promotionPiece);
            
            NetworkMessage response;
            response.type = MessageType::MOVE_RESPONSE;
            response.success = valid;
            response.moveData = msg.moveData;

            logMessage(QString("In MessageType::MOVE, sender: %1, makeMove response.success: %2, response.moveData: %3")
                            .arg(sender->peerAddress().toString())
                            .arg(response.success)
                            .arg(response.moveData.toString()));

            if (valid) {
                broadcastMessage(response);
                
                if (game_->isCheckmate(game_->getCurrentTurn())) {
                    NetworkMessage endMsg;
                    endMsg.type = MessageType::GAME_END;
                    endMsg.data = tr("%1 wins by checkmate!")
                        .arg(playerColor == PieceColor::White ? "White" : "Black");
                    logMessage(QString("In MessageType::MOVE, sender: %1, broadcastMessage.type: %2, broadcastMessage.data: %3")
                            .arg(sender->peerAddress().toString())
                            .arg(messageTypeToString(endMsg.type))
                            .arg(endMsg.data));
                    broadcastMessage(endMsg);
                    emit gameEnded(endMsg.data);
                }
                else if (game_->isStalemate(game_->getCurrentTurn())) {
                    NetworkMessage endMsg;
                    endMsg.type = MessageType::GAME_END;
                    endMsg.data = "Game drawn by stalemate";
                    logMessage(QString("In MessageType::MOVE, sender: %1, broadcastMessage.type: %2, broadcastMessage.data: %3")
                            .arg(sender->peerAddress().toString())
                            .arg(messageTypeToString(endMsg.type))
                            .arg(endMsg.data));
                    broadcastMessage(endMsg);
                    emit gameEnded(endMsg.data);
                }
            } else {
                response.data = "Invalid move";
                logMessage(QString("In MessageType::MOVE, sender: %1, response.data: %2")
                            .arg(sender->peerAddress().toString())
                            .arg(response.data));
                sendMessage(sender, response);
            }
            break;
        }

        case MessageType::MOVE_RESPONSE: {
            // Server shouldn't receive this
            logMessage(QString("In MessageType::MOVE_RESPONSE, UNEXPECTED Message - sender: %1").arg(sender->peerAddress().toString()));
            break;
        }
            
        case MessageType::DRAW_OFFER: {
            // Your existing DRAW_OFFER case implementation
            logMessage(QString("In MessageType::DRAW_OFFER, sender: %1, msg: %2" )
                            .arg(sender->peerAddress().toString())
                            .arg(msg.data));
            QTcpSocket* opponent = (sender == clients_[0]) ? clients_[1] : clients_[0];
            if (opponent)
            {
                sendMessage(opponent, msg);
            }
            break;
        }
        
        case MessageType::DRAW_RESPONSE: {
            // Your existing DRAW_RESPONSE case implementation
            logMessage(QString("In MessageType::DRAW_RESPONSE, sender: %1, msg: %2" )
                            .arg(sender->peerAddress().toString())
                            .arg(msg.data));

            QTcpSocket* opponent = (sender == clients_[0]) ? clients_[1] : clients_[0];
            if (opponent)
            {
                sendMessage(opponent, msg);
            }
            
            if (msg.success) {
                NetworkMessage endMsg;
                endMsg.type = MessageType::GAME_END;
                endMsg.data = "Game drawn by agreement";
                logMessage(QString("In MessageType::DRAW_RESPONSE, sender: %1, broadcastMessage.type: %2, broadcastMessage.data: %3")
                            .arg(sender->peerAddress().toString())
                            .arg(messageTypeToString(endMsg.type))
                            .arg(endMsg.data));
                broadcastMessage(endMsg);
                emit gameEnded(endMsg.data);
            }
            break;
        }
        
        case MessageType::RESIGN: {
            // Your existing RESIGN case implementation
            logMessage(QString("In MessageType::RESIGN, sender: %1, msg: %2")
                            .arg(sender->peerAddress().toString())
                            .arg(msg.data));
            NetworkMessage endMsg;
            endMsg.type = MessageType::GAME_END;
            endMsg.data = tr("%1 resigns. %2 wins!")
                .arg(playerColor == PieceColor::White ? "White" : "Black")
                .arg(playerColor == PieceColor::White ? "Black" : "White");
            logMessage(QString("In MessageType::RESIGN, sender: %1, broadcastMessage.type: %2, broadcastMessage.data: %3")
                            .arg(sender->peerAddress().toString())
                            .arg(messageTypeToString(endMsg.type))
                            .arg(endMsg.data));
            broadcastMessage(endMsg);
            emit gameEnded(endMsg.data);
            break;
        }
        
        case MessageType::CHAT: {
            // Your existing CHAT case implementation
            logMessage(QString("In MessageType::CHAT, sender: %1, msg: %2")
                            .arg(sender->peerAddress().toString())
                            .arg(msg.data));
            QTcpSocket* opponent = (sender == clients_[0]) ? clients_[1] : clients_[0];
            if (opponent)
            {
                sendMessage(opponent, msg);
            }
            break;
        }

        case MessageType::ERROR: {
            // Forward error messages to the appropriate client
            logMessage(QString("In MessageType::ERROR, sender: %1, msg: %2")
                            .arg(sender->peerAddress().toString())
                            .arg(msg.data));
            if (msg.data.contains("WHITE")) {
                sendMessage(clients_[0], msg);
            } else if (msg.data.contains("BLACK")) {
                sendMessage(clients_[1], msg);
            } else {
                broadcastMessage(msg);
            }
            break;
        }

        case MessageType::KEEPALIVE: {
            // Just acknowledge receipt
            logMessage(QString("In MessageType::KEEPALIVE, sender: %1, msg: %2")
                            .arg(sender->peerAddress().toString())
                            .arg(msg.data));
            sendMessage(sender, msg);
            break;
        }

        default: {
            // Handle unknown message type
            logMessage(QString("In MessageType::UNKNOWN, sender: %1, msg: %2")
                            .arg(sender->peerAddress().toString())
                            .arg(msg.data));
            NetworkMessage errorMsg;
            errorMsg.type = MessageType::ERROR;
            errorMsg.data = "Unknown message type received";
            logMessage(QString("In MessageType::UNKNOWN, sender: %1, errorMsg: %2")
                            .arg(sender->peerAddress().toString())
                            .arg(errorMsg.data));
            sendMessage(sender, errorMsg);
            break;
        }
    }
}

void ChessNetworkServer::cleanupClient(QTcpSocket* client) {
    for (int i = 0; i < 2; ++i) {
        if (clients_[i] == client) {
            logMessage(QString("Cleaning up client at slot %1.").arg(i));
            clients_[i] = nullptr;
            emit clientDisconnected(i == 0 ? PieceColor::White : PieceColor::Black);
            
            // Notify remaining client
            if (clients_[1-i]) {
                NetworkMessage msg;
                msg.type = MessageType::GAME_END;
                msg.data = "Opponent disconnected";
                logMessage(QString("In cleanupClient, sender: %1, broadcastMessage.type: %2, broadcastMessage.data: %3")
                            .arg(clients_[1-i]->peerAddress().toString())
                            .arg(messageTypeToString(msg.type))
                            .arg(msg.data));
                sendMessage(clients_[1-i], msg);
            }
            break;
        }
    }
    
    client->deleteLater();
}