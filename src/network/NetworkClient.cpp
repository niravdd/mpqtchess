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
    socket_->write(data);
}

void NetworkClient::onConnected()
{
    emit connected();
}

void NetworkClient::onDisconnected()
{
    emit disconnected();
}

void NetworkClient::onReadyRead()
{
    buffer_ += socket_->readAll();
    
    int endIndex;
    while ((endIndex = buffer_.indexOf('\n')) != -1) {
        QByteArray jsonData = buffer_.left(endIndex);
        buffer_.remove(0, endIndex + 1);
                
        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseError);
        
        if (parseError.error != ::QJsonParseError::NoError) {
            emit errorOccurred("Invalid message from server: " + parseError.errorString());
            continue;
        }
        
        if (!doc.isObject()) {
            emit errorOccurred("Message is not a JSON object");
            continue;
        }
        
        QJsonObject obj = doc.object();
        
        if (obj["type"].toString() == "move") {
            QString from = obj["from"].toString();
            QString to = obj["to"].toString();
            
            if (!from.isEmpty() && !to.isEmpty()) {
                emit moveReceived(jsonData);
            }
        }
    }
}

void NetworkClient::onError(QAbstractSocket::SocketError socketError)
{
    Q_UNUSED(socketError);                  // Add this if you don't use the parameter directly
    emit errorOccurred("Socket error: " + socket_->errorString());
}