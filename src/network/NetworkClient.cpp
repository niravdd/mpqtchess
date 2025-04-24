// /src/network/NetworkClient.cpp
#include "NetworkClient.h"
#include <QJsonDocument>
#include <QJsonObject>

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

    qDebug() << "from NetworkClient::sendMove(): data = " << data;

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
    if (socket_ && socket_->state() == QTcpSocket::ConnectedState)
    {
        socket_->write(data);
        socket_->flush();
    } else
    {
        emit errorOccurred("Not connected to server");
    }
}

void NetworkClient::sendReadyStatus()
{
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
}

void NetworkClient::onReadyRead()
{
    buffer_ += socket_->readAll();
    
    int endIndex;
    while ((endIndex = buffer_.indexOf('\n')) != -1) {
        const QByteArray rawData = buffer_.left(endIndex);
        buffer_.remove(0, endIndex + 1);
        
        // Emit raw data for generic handling
        emit rawDataReceived(rawData);

        // Parse for moves
        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(rawData, &parseError);
        
        if (parseError.error == QJsonParseError::NoError && doc.isObject()) {
            QJsonObject obj = doc.object();
            
            if (obj["type"].toString() == "move") {
                int fromCol, fromRow, toCol, toRow;
                if (parseMoveData(obj, fromCol, fromRow, toCol, toRow)) {
                    emit parsedMoveReceived(fromCol, fromRow, toCol, toRow);
                }
            }
        }
    }
}

// New move parsing function
bool NetworkClient::parseMoveData(const QJsonObject& obj, 
                                 int& fromCol, int& fromRow,
                                 int& toCol, int& toRow)
{
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
    return true;
}

void NetworkClient::onError(QAbstractSocket::SocketError socketError)
{
    Q_UNUSED(socketError);                  // Add this if you don't use the parameter directly
    emit errorOccurred("Socket error: " + socket_->errorString());
}

void NetworkClient::processNetworkData(const QByteArray& data)
{
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
}