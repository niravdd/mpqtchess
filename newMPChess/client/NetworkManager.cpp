// NetworkManager.cpp
#include "NetworkManager.h"
#include <QDebug>

NetworkManager& NetworkManager::getInstance() {
    static NetworkManager instance;
    return instance;
}

NetworkManager::NetworkManager(QObject* parent) 
    : QObject(parent), socket(new QTcpSocket(this)) {
    
    connect(socket, &QTcpSocket::connected, this, &NetworkManager::onSocketConnected);
    connect(socket, &QTcpSocket::disconnected, this, &NetworkManager::onSocketDisconnected);
    connect(socket, &QTcpSocket::errorOccurred, this, &NetworkManager::onSocketError);
    connect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
    
    // Set up ping timer
    connect(&pingTimer, &QTimer::timeout, this, &NetworkManager::sendPing);
    pingTimer.setInterval(30000); // 30 seconds
}

NetworkManager::~NetworkManager() {
    disconnectFromServer();
}

bool NetworkManager::connectToServer(const QString& host, quint16 port) {
    if (socket->state() != QAbstractSocket::UnconnectedState) {
        disconnectFromServer();
    }
    
    socket->connectToHost(host, port);
    return socket->waitForConnected(5000); // 5 second timeout
}

void NetworkManager::disconnectFromServer() {
    pingTimer.stop();
    
    if (socket->state() != QAbstractSocket::UnconnectedState) {
        socket->disconnectFromHost();
        if (socket->state() != QAbstractSocket::UnconnectedState) {
            socket->abort();
        }
    }
}

bool NetworkManager::isConnected() const {
    return socket->state() == QAbstractSocket::ConnectedState;
}

bool NetworkManager::sendMessage(const Message& message) {
    // Serialize message
    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    stream.setVersion(QDataStream::Qt_6_5);
    
    // Write the message type
    stream << static_cast<quint32>(message.type);
    
    // Write the payload
    stream << message.payload;
    
    return sendRawMessage(data);
}

bool NetworkManager::sendRawMessage(const QByteArray& data) {
    if (!isConnected()) {
        return false;
    }
    
    // Add a header with the message length
    QByteArray packet;
    QDataStream headerStream(&packet, QIODevice::WriteOnly);
    headerStream.setVersion(QDataStream::Qt_6_5);
    
    // Write the message length as a 4-byte integer
    headerStream << static_cast<quint32>(data.size());
    
    // Append the actual message data
    packet.append(data);
    
    // Send the packet
    qint64 bytesSent = socket->write(packet);
    return bytesSent == packet.size();
}

void NetworkManager::sendPing() {
    if (isConnected()) {
        Message pingMessage;
        pingMessage.type = MessageType::PING;
        sendMessage(pingMessage);
    }
}

void NetworkManager::onSocketConnected() {
    qDebug() << "Connected to server";
    pingTimer.start();
    emit connected();
}

void NetworkManager::onSocketDisconnected() {
    qDebug() << "Disconnected from server";
    pingTimer.stop();
    emit disconnected();
}

void NetworkManager::onSocketError(QAbstractSocket::SocketError socketError) {
    QString errorMessage = socket->errorString();
    qDebug() << "Socket error:" << errorMessage;
    emit errorOccurred(errorMessage);
}

void NetworkManager::onReadyRead() {
    static QByteArray buffer;
    static quint32 expectedSize = 0;
    static bool readingHeader = true;
    
    // Keep reading while there's data available
    while (socket->bytesAvailable() > 0) {
        if (readingHeader) {
            // Need at least 4 bytes to read the header
            if (socket->bytesAvailable() < 4) {
                break;
            }
            
            // Read the message size header
            QByteArray headerData = socket->read(4);
            QDataStream headerStream(headerData);
            headerStream.setVersion(QDataStream::Qt_6_5);
            headerStream >> expectedSize;
            
            // Now we're reading the message body
            readingHeader = false;
            buffer.clear();
        }
        
        // Read message body
        if (!readingHeader) {
            // Read available data up to expected size
            quint32 bytesToRead = qMin(socket->bytesAvailable(), static_cast<qint64>(expectedSize - buffer.size()));
            buffer.append(socket->read(bytesToRead));
            
            // Check if we've got the complete message
            if (buffer.size() == expectedSize) {
                // Message is complete, parse it and emit signal
                Message message = parseMessage(buffer);
                
                QMutexLocker locker(&messageMutex);
                incomingMessages.enqueue(message);
                
                // Process special messages immediately, queue the rest
                if (message.type == MessageType::PONG) {
                    // Handle pong messages directly
                } else {
                    QMetaObject::invokeMethod(this, &NetworkManager::processMessages, Qt::QueuedConnection);
                }
                
                // Reset for next message
                readingHeader = true;
                expectedSize = 0;
                buffer.clear();
            }
        }
    }
}

Message NetworkManager::parseMessage(const QByteArray& data) {
    Message message;
    QDataStream stream(data);
    stream.setVersion(QDataStream::Qt_6_5);
    
    quint32 messageTypeInt;
    stream >> messageTypeInt;
    message.type = static_cast<MessageType>(messageTypeInt);
    
    stream >> message.payload;
    
    return message;
}

void NetworkManager::processMessages() {
    QMutexLocker locker(&messageMutex);
    
    while (!incomingMessages.isEmpty()) {
        Message message = incomingMessages.dequeue();
        emit messageReceived(message);
    }
}