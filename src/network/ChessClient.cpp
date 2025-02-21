// src/network/ChessClient.cpp
#include "ChessClient.h"
#include <QtCore/QDataStream>
#include <QtCore/QTimer>

ChessClient::ChessClient(QObject* parent)
    : QObject(parent)
    , playerColor_(PieceColor::White)
    , gameInProgress_(false)
    , localGame_(std::make_unique<ChessGame>())
    , myTurn_(false)
    , drawPending_(false)
{
    // Set up socket connections
    connect(&socket_, &QTcpSocket::connected,
            this, &ChessClient::handleConnected);
    connect(&socket_, &QTcpSocket::disconnected,
            this, &ChessClient::handleDisconnected);
    connect(&socket_, &QTcpSocket::readyRead,
            this, &ChessClient::handleReadyRead);
/// connect(&socket_, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::error), this, &ChessClient::handleError);
    connect(&socket_, &QTcpSocket::errorOccurred,
        this, &ChessClient::handleError);

    // Set up keep-alive timer
    QTimer* keepAliveTimer = new QTimer(this);
    connect(keepAliveTimer, &QTimer::timeout, [this]() {
        if (isConnected()) {
            NetworkMessage msg;
            msg.type = MessageType::KEEPALIVE;
            sendMessage(msg);
        }
    });
    keepAliveTimer->start(KEEPALIVE_INTERVAL);
}

ChessClient::~ChessClient()
{
    if (isConnected()) {
        socket_.disconnectFromHost();
    }
}

bool ChessClient::connectToServer(const QString& address, quint16 port)
{
    if (isConnected()) {
        return true;
    }

    socket_.connectToHost(address, port);
    return socket_.waitForConnected(CONNECTION_TIMEOUT);
}

void ChessClient::disconnect()
{
    if (isConnected()) {
        socket_.disconnectFromHost();
    }
}

void ChessClient::sendMove(const Position& from, const Position& to, 
                          PieceType promotionPiece)
{
    if (!gameInProgress_ || !myTurn_) {
        emit moveValidated(false, ERR_NOT_YOUR_TURN);
        return;
    }

    if (!validateMove(from, to)) {
        emit moveValidated(false, ERR_INVALID_MOVE);
        return;
    }

    NetworkMessage msg;
    msg.type = MessageType::MOVE;
    msg.moveData.from = from;
    msg.moveData.to = to;
    msg.moveData.promotionPiece = promotionPiece;
    sendMessage(msg);
}

void ChessClient::offerDraw()
{
    if (!gameInProgress_) {
        return;
    }

    NetworkMessage msg;
    msg.type = MessageType::DRAW_OFFER;
    sendMessage(msg);
}

void ChessClient::respondToDraw(bool accept)
{
    if (!drawPending_) {
        return;
    }

    NetworkMessage msg;
    msg.type = MessageType::DRAW_RESPONSE;
    msg.data = accept ? "accept" : "decline";
    sendMessage(msg);
    drawPending_ = false;
}

void ChessClient::resign()
{
    if (!gameInProgress_) {
        return;
    }

    NetworkMessage msg;
    msg.type = MessageType::RESIGN;
    sendMessage(msg);
}

void ChessClient::sendChatMessage(const QString& message)
{
    if (!isConnected()) {
        return;
    }

    NetworkMessage msg;
    msg.type = MessageType::CHAT;
    msg.data = message;
    sendMessage(msg);
}

void ChessClient::handleConnected()
{
    receivedData_.clear();
    
    // Send initial connection request
    NetworkMessage msg;
    msg.type = MessageType::CONNECT_REQUEST;
    sendMessage(msg);
}

void ChessClient::handleDisconnected()
{
    gameInProgress_ = false;
    myTurn_ = false;
    drawPending_ = false;
    playerColor_ = PieceColor::None;
    localGame_ = std::make_unique<ChessGame>();
    emit disconnected();
}

void ChessClient::handleError([[maybe_unused]] QAbstractSocket::SocketError socketError)
{
    QString errorMessage = socket_.errorString();
    emit connectionError(errorMessage);
    
    if (socket_.state() != QAbstractSocket::ConnectedState) {
        handleDisconnected();
    }
}

void ChessClient::handleReadyRead()
{
    appendToBuffer(socket_.readAll());
    
    while (tryProcessNextMessage()) {
        // Continue processing messages until buffer is empty
        // or we don't have a complete message
    }
}

void ChessClient::appendToBuffer(const QByteArray& data)
{
    if (receivedData_.size() + data.size() > MAX_MESSAGE_SIZE) {
        handleProtocolError("Message size exceeds maximum allowed");
        socket_.disconnectFromHost();
        return;
    }
    receivedData_.append(data);
}

bool ChessClient::tryProcessNextMessage()
{
    if (receivedData_.size() < HEADER_SIZE) {
        return false;
    }

    QDataStream stream(receivedData_);
    stream.setVersion(QDataStream::Qt_5_15);

    // Read message size
    quint32 messageSize;
    stream >> messageSize;

    if (messageSize > MAX_MESSAGE_SIZE) {
        handleProtocolError("Invalid message size");
        socket_.disconnectFromHost();
        return false;
    }

    // Check if we have the complete message
    if (receivedData_.size() < HEADER_SIZE + messageSize) {
        return false;
    }

    // Extract the message data
    QByteArray messageData = receivedData_.mid(HEADER_SIZE, messageSize);
    receivedData_.remove(0, HEADER_SIZE + messageSize);

    // Process the message
    NetworkMessage msg = parseMessage(messageData);
    processMessage(msg);

    return !receivedData_.isEmpty();
}

NetworkMessage ChessClient::parseMessage(const QByteArray& data)
{
    NetworkMessage msg;
    QDataStream stream(data);
    stream.setVersion(QDataStream::Qt_5_15);
    
    stream >> msg;
    return msg;
}

void ChessClient::sendMessage(const NetworkMessage& msg)
{
    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    stream.setVersion(QDataStream::Qt_5_15);

    // Reserve space for size
    stream << quint32(0);
    
    // Write message
    stream << msg;

    // Write actual size
    stream.device()->seek(0);
    stream << quint32(data.size() - sizeof(quint32));

    // Send the message
    socket_.write(data);
}

void ChessClient::processMessage(const NetworkMessage& msg)
{
    switch (msg.type) {
        case MessageType::CONNECT_REQUEST:
            // Client shouldn't receive this - server-bound message
            handleProtocolError("Unexpected CONNECT_REQUEST received");
            break;

        case MessageType::CONNECT_RESPONSE:
            processConnectResponse(msg);
            break;
            
        case MessageType::GAME_STATE:
            processGameStateMessage(msg);
            break;
            
        case MessageType::MOVE:
            processMoveMessage(msg);
            break;

        case MessageType::MOVE_RESPONSE:
            processMoveMessage(msg);
            break;
            
        case MessageType::DRAW_OFFER:
            processPlayerActionMessage(msg);
            break;

        case MessageType::DRAW_RESPONSE:
            processPlayerActionMessage(msg);
            break;

        case MessageType::RESIGN:
            processPlayerActionMessage(msg);
            break;
            
        case MessageType::CHAT:
            emit chatMessageReceived(msg.data);
            break;
            
        case MessageType::ERROR:
            handleProtocolError(msg.data);
            break;
            
        case MessageType::KEEPALIVE:
            // Just ignore keepalive messages
            break;

        case MessageType::GAME_END:
            // Process game end notification
            processGameStateMessage(msg);
            break;
            
        default:
            handleProtocolError("Unknown message type received");
            break;
    }
}

void ChessClient::processConnectResponse(const NetworkMessage& msg)
{
    if (msg.success) {
        playerColor_ = msg.data == "WHITE" ? PieceColor::White : PieceColor::Black;
        emit connected(playerColor_);
    } else {
        handleProtocolError("Connection rejected: " + msg.data);
        socket_.disconnectFromHost();
    }
}

void ChessClient::processGameStateMessage(const NetworkMessage& msg)
{
    if (!isValidGameState(msg.data)) {
        handleProtocolError("Invalid game state received");
        return;
    }

    if (msg.data == "started") {
        gameInProgress_ = true;
        myTurn_ = (playerColor_ == PieceColor::White);
        localGame_ = std::make_unique<ChessGame>();
        emit gameStarted();
        emit turnChanged(PieceColor::White);
    }
    else if (msg.data == "ended") {
        gameInProgress_ = false;
        emit gameEnded(msg.extraData);
    }
}

void ChessClient::processMoveMessage(const NetworkMessage& msg)
{
    if (msg.type == MessageType::MOVE) {
        // Opponent's move
        if (!validateMove(msg.moveData.from, msg.moveData.to)) {
            handleProtocolError("Invalid move received");
            return;
        }

        localGame_->makeMove(msg.moveData.from, msg.moveData.to,
                           playerColor_ == PieceColor::White ? 
                           PieceColor::Black : PieceColor::White,
                           msg.moveData.promotionPiece);

        emit moveReceived(msg.moveData.from, msg.moveData.to,
                        msg.moveData.promotionPiece);
        myTurn_ = true;
        emit turnChanged(playerColor_);
    }
    else { // MOVE_RESPONSE
        if (msg.success) {
            localGame_->makeMove(msg.moveData.from, msg.moveData.to,
                               playerColor_,
                               msg.moveData.promotionPiece);
            
            emit moveValidated(true, "");
            emit moveMade(msg.moveData.from, msg.moveData.to);
            myTurn_ = false;
            emit turnChanged(playerColor_ == PieceColor::White ? 
                           PieceColor::Black : PieceColor::White);
        }
        else {
            emit moveValidated(false, msg.data);
        }
    }

    // Check for special conditions after move
    if (localGame_->isInCheck(playerColor_)) {
        emit checkOccurred();
    }
    if (localGame_->isCheckmate(playerColor_)) {
        emit checkmateOccurred(playerColor_ == PieceColor::White ? 
                              PieceColor::Black : PieceColor::White);
    }
    if (localGame_->isStalemate(playerColor_)) {
        emit stalemateOccurred();
    }
}

void ChessClient::processPlayerActionMessage(const NetworkMessage& msg)
{
    switch (msg.type) {
        case MessageType::CONNECT_REQUEST:
            handleProtocolError("Unexpected message type");
            break;
            
        case MessageType::CONNECT_RESPONSE:
            handleProtocolError("Unexpected message type");
            break;
            
        case MessageType::GAME_STATE:
            handleProtocolError("Unexpected message type");
            break;
            
        case MessageType::MOVE:
            handleProtocolError("Unexpected message type");
            break;
            
        case MessageType::MOVE_RESPONSE:
            handleProtocolError("Unexpected message type");
            break;
            
        case MessageType::DRAW_OFFER:
            drawPending_ = true;
            emit drawOffered();
            break;
            
        case MessageType::DRAW_RESPONSE:
            drawPending_ = false;
            if (msg.data == "accept") {
                gameInProgress_ = false;
                emit drawResponseReceived(true);
                emit drawOccurred("Draw by agreement");
            } else {
                emit drawResponseReceived(false);
            }
            break;
            
        case MessageType::RESIGN:
            gameInProgress_ = false;
            emit playerResigned(playerColor_ == PieceColor::White ? 
                              PieceColor::Black : PieceColor::White);
            emit gameEnded("Resignation");
            break;
            
        case MessageType::CHAT:
            handleProtocolError("Unexpected message type");
            break;
            
        case MessageType::ERROR:
            handleProtocolError("Unexpected message type");
            break;
            
        case MessageType::KEEPALIVE:
            handleProtocolError("Unexpected message type");
            break;
            
        case MessageType::GAME_END:
            handleProtocolError("Unexpected message type");
            break;
            
        default:
            handleProtocolError("Unknown message type");
            break;
    }
}

bool ChessClient::validateMove(const Position& from, const Position& to) const
{
    if (!localGame_) {
        return false;
    }
    
    return localGame_->isValidMove(from, to, 
                                 myTurn_ ? playerColor_ : 
                                 (playerColor_ == PieceColor::White ? 
                                  PieceColor::Black : PieceColor::White));
}

bool ChessClient::isValidGameState(const QString& state) const
{
    return VALID_GAME_STATES.contains(state);
}

void ChessClient::handleProtocolError(const QString& error)
{
    emit connectionError(ERR_PROTOCOL_ERROR + ": " + error);
}

void ChessClient::handleNetworkError(const QString& error)
{
    emit connectionError(error);
}