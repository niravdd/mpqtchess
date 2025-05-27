// /src/network/NetworkClient.cpp
#include "NetworkClient.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>

NetworkClient::NetworkClient(QObject* parent)
    : QObject(parent)
    , socket_(new QTcpSocket(this))
{
    connect(socket_, &::QTcpSocket::connected, this, &NetworkClient::onConnected);
    connect(socket_, &::QTcpSocket::disconnected, this, &NetworkClient::onDisconnected);
    connect(socket_, &::QTcpSocket::readyRead, this, &NetworkClient::onReadyRead);
    connect(socket_, &::QTcpSocket::errorOccurred, this, &NetworkClient::onError);
}

NetworkClient::~NetworkClient()
{
    if (socket_->state() == ::QAbstractSocket::ConnectedState) {
        socket_->disconnectFromHost();
    }
}

bool NetworkClient::connectToServer(const QString& host, int port)
{
    if (socket_->state() == ::QAbstractSocket::ConnectedState) {
        return true;
    }

    socket_->connectToHost(host, port);
    return socket_->waitForConnected(5000);
}

void NetworkClient::disconnectFromServer()
{
    if (socket_->state() == ::QAbstractSocket::ConnectedState) {
        socket_->disconnectFromHost();
    }
}

bool NetworkClient::isConnected() const
{
    return socket_->state() == ::QAbstractSocket::ConnectedState;
}

void NetworkClient::sendMove(const QString& from, const QString& to)
{
    qDebug() << "from NetworkClient::sendMove(): entered...";
    if (!isConnected()) {
        emit errorOccurred("Not connected to server");
        return;
    }

    QJsonObject moveObj;
    moveObj["type"] = "move";
    moveObj["from"] = from;
    moveObj["to"] = to;

    QJsonDocument doc(moveObj);
    QByteArray data = doc.toJson(::QJsonDocument::Compact) + "\n";

    qDebug() << "from NetworkClient::sendMove(): data = { " << data << " }";

    socket_->write(data);

    qDebug() << "from NetworkClient::sendMove(): data sent, exit...";
}

void NetworkClient::onConnected()
{
    emit connected();
}

void NetworkClient::onDisconnected()
{
    emit disconnected();
}

void NetworkClient::sendData(const QByteArray& data)
{
    qDebug() << "from NetworkClient::sendData(): entered...";
    qDebug() << "from NetworkClient::sendData(): data = { " << data << " }";

    if (socket_ && socket_->state() == QTcpSocket::ConnectedState)
    {
        socket_->write(data);
        socket_->flush();
    } else
    {
        emit errorOccurred("Not connected to server");
    }

    qDebug() << "from NetworkClient::sendData(): exit...";
}

void NetworkClient::sendReadyStatus()
{
    qDebug() << "from NetworkClient::sendReadyStatus(): entered...";

    if (!socket_ || socket_->state() != QTcpSocket::ConnectedState) {
        emit errorOccurred("Not connected to server");
        return;
    }
    
    NetworkMessage msg;
    msg.type = MessageType::PLAYER_READY;
    msg.data = "READY";
    
    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    stream.setVersion(QDataStream::Qt_5_15);
    stream << msg;
    
    socket_->write(data);

    qDebug() << "from NetworkClient::sendReadyStatus(): exit...";
}

void NetworkClient::onReadyRead()
{
    qDebug() << "from NetworkClient::onReadyRead(): NetworkClient received data from server";

    QByteArray newData = socket_->readAll();
    qDebug() << "from NetworkClient::onReadyRead(): New data received (size):" << newData.size() << "bytes";
    
    buffer_ += newData;
    qDebug() << "from NetworkClient::onReadyRead(): Current buffer size:" << buffer_.size() << "bytes";
    qDebug() << "from NetworkClient::onReadyRead(): Buffer as hex:" << buffer_.toHex();

    // First try to parse as JSON for backward compatibility
    int endIndex = buffer_.indexOf('\n');
    if (endIndex != -1) {
        QByteArray jsonData = buffer_.left(endIndex);
        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseError);
        
        if (parseError.error == QJsonParseError::NoError && doc.isObject()) {
            // Process JSON message
            buffer_.remove(0, endIndex + 1);
            emit rawDataReceived(jsonData);
            return;
        }
    }

    // Process binary messages from the buffer
    while (buffer_.size() >= 4) {  // Assuming at least 4 bytes for header
        // Assuming first 4 bytes contain message type
        int messageType = *reinterpret_cast<const int*>(buffer_.constData());
        
        qDebug() << "from NetworkClient::onReadyRead(): Message type detected: " << messageType;
        
        // Process different message types
        switch (messageType) {
            case static_cast<int>(MessageType::PLAYER_READY): {
                if (buffer_.size() < 8) return; // Wait for more data
                
                // Process PLAYER_READY message
                qDebug() << "from NetworkClient::onReadyRead(): Processing PLAYER_READY message...";
                
                // Extract string data
                int stringLength = *reinterpret_cast<const int*>(buffer_.constData() + 4);
                if (buffer_.size() < 8 + stringLength) return; // Wait for more data
                
                // Use a safer approach for string conversion
                QByteArray stringData(buffer_.mid(8, stringLength * 2)); // UTF-16 = 2 bytes per char
                QString message = QString::fromUtf8(stringData);
                
                qDebug() << "from NetworkClient::onReadyRead(): Connected & PLAYER_READY message content:" << message;
                
                // Remove processed message from buffer
                buffer_.remove(0, 8 + stringLength);
                break;
            }
            
            case static_cast<int>(MessageType::GAME_START): {
                if (buffer_.size() < 8)
                    return; // Wait for more data

                // Process GAME_START message
                qDebug() << "from NetworkClient::onReadyRead(): Processing GAME_START message...";
                
                // Extract color data
                int stringLength = *reinterpret_cast<const int*>(buffer_.constData() + 4);
                if(buffer_.size() < 8 + stringLength) return; // Wait for more data
                
                // Use a safer approach for string conversion
                QByteArray stringData(buffer_.mid(8, stringLength * 2)); // UTF-16 = 2 bytes per char
                QString colorString = QString::fromUtf8(stringData);
                
                qDebug() << "from NetworkClient::onReadyRead(): Color assignment: " << colorString;
                
                PieceColor color = PieceColor::None;
                if(colorString == "WHITE")
                {
                    color = PieceColor::White;
                    qDebug() << "from NetworkClient::onReadyRead(): Assigned WHITE color to player";
                } else if(colorString == "BLACK")
                {
                    color = PieceColor::Black;
                    qDebug() << "from NetworkClient::onReadyRead(): Assigned BLACK color to player";
                } else {
                    qDebug() << "from NetworkClient::onReadyRead(): WARNING: Unknown color string: " << colorString;
                }
                
                // Remove processed message from buffer
                buffer_.remove(0, 8 + stringLength);
                
                // Emit the signal for color assignment immediately
                if (color != PieceColor::None) {
                    qDebug() << "from NetworkClient::onReadyRead(): Emitting colorAssigned signal with color: " 
                             << (color == PieceColor::White ? "White" : "Black");
                    
                    // Emit the color assignment signal directly
                    emit colorAssigned(color);
                }
                break;
            }
            
            default:
                // Unknown message type - try to recover
                qDebug() << "from NetworkClient::onReadyRead(): Unknown message type:" << messageType;
                buffer_.remove(0, 1); // Remove one byte and try again
                break;
        }
    }

    qDebug() << "from NetworkClient::onReadyRead(): Remaining buffer size:" << buffer_.size() << "bytes";
    qDebug() << "from NetworkClient::onReadyRead(): exit...";
}

// void NetworkClient::onReadyRead()
// {
//     qDebug() << "from NetworkClient::onReadyRead(): NetworkClient received data from server";

//     buffer_ += socket_->readAll();

//     qDebug() << "from NetworkClient::onReadyRead(): buffer_ = { " << buffer_ << " } ";
    
//     int endIndex;
//     while ((endIndex = buffer_.indexOf('\n')) != -1) {
//         const QByteArray rawData = buffer_.left(endIndex);
//         buffer_.remove(0, endIndex + 1);
        
//         // Emit raw data for generic handling
//         emit rawDataReceived(rawData);

//         // Parse for moves
//         QJsonParseError parseError;
//         QJsonDocument doc = QJsonDocument::fromJson(rawData, &parseError);

//         qDebug() << "from NetworkClient::onReadyRead(): doc = { " << doc.toJson(QJsonDocument::Indented) << " } ";

//         if (parseError.error == QJsonParseError::NoError && doc.isObject()) {
//             QJsonObject obj = doc.object();
            
//             if (obj["type"].toString() == "move") {
//                 int fromCol, fromRow, toCol, toRow;
//                 if (parseMoveData(obj, fromCol, fromRow, toCol, toRow)) {
//                     emit parsedMoveReceived(fromCol, fromRow, toCol, toRow);
//                 }
//             }
//         }
//     }

//     qDebug() << "from NetworkClient::onReadyRead(): exit...";
// }

// New move parsing function
bool NetworkClient::parseMoveData(const QJsonObject& obj, 
                                 int& fromCol, int& fromRow,
                                 int& toCol, int& toRow)
{
    qDebug() << "from NetworkClient::parseMoveData(): entered...";

    auto convertNotation = [](const QString& notation) -> std::pair<int, int> {
        if (notation.length() != 2) return {-1, -1};
        
        QChar fileChar = notation[0].toLower();
        QChar rankChar = notation[1];
        
        int col = fileChar.unicode() - 'a';  // a=0, h=7
        int row = 7 - (rankChar.unicode() - '1');  // Convert to 0-based row index
        
        if (col < 0 || col > 7 || row < 0 || row > 7) {
            return {-1, -1};
        }
        return {col, row};
    };

    QString fromStr = obj["from"].toString();
    QString toStr = obj["to"].toString();
    
    auto [fc, fr] = convertNotation(fromStr);
    auto [tc, tr] = convertNotation(toStr);
    
    if (fc == -1 || fr == -1 || tc == -1 || tr == -1) {
        emit errorOccurred("Invalid move format: " + fromStr + "->" + toStr);
        return false;
    }

    fromCol = fc;
    fromRow = fr;
    toCol = tc;
    toRow = tr;

    qDebug() << "from NetworkClient::parseMoveData(): exit...";

    return true;
}

void NetworkClient::onError(QAbstractSocket::SocketError socketError)
{
    Q_UNUSED(socketError);                  // Add this if you don't use the parameter directly
    emit errorOccurred("Socket error: " + socket_->errorString());
}

void NetworkClient::processNetworkData(const QByteArray& data)
{
    qDebug() << "from NetworkClient::processNetworkData(): entered...";
    qDebug() << "from NetworkClient::processNetworkData(): data = { " << data << " }";

    // Example format: "e2-e4" or "g1-f3"
    QString moveStr = QString::fromUtf8(data).simplified();
    
    // Basic validation
    if (moveStr.length() < 5 || !moveStr.contains('-')) {
        qWarning() << "Invalid move format received:" << moveStr;
        return;
    }

    // Split into from/to components
    QStringList parts = moveStr.split('-');
    if (parts.size() != 2) {
        qWarning() << "Malformed move string:" << moveStr;
        return;
    }

    QString from = parts[0].trimmed().left(2);
    QString to = parts[1].trimmed().left(2);

    // Basic chess notation validation
    QRegularExpression squareRegex("^[a-h][1-8]\\$");
    if (!squareRegex.match(from).hasMatch() || !squareRegex.match(to).hasMatch()) {
        qWarning() << "Invalid chess coordinates:" << from << "->" << to;
        return;
    }

    emit moveReceived(from, to);

    qDebug() << "from NetworkClient::processNetworkData(): exit...";
}