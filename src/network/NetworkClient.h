// /src/network/NetworkClient.h
#pragma once

// Include compatibility header first
#include "../QtCompat.h"
#include <QObject>
#include <QtCore/QtGlobal>
#include <QtNetwork/QTcpSocket>
#include <QString>

class NetworkClient : public QObject {
    Q_OBJECT

public:
    explicit NetworkClient(QObject* parent = nullptr);
    ~NetworkClient();

    bool connectToServer(const QString& host, int port);
    void disconnectFromServer();
    bool isConnected() const;
    void sendMove(const QString& from, const QString& to);

signals:
    void connected();
    void disconnected();
    void moveReceived(const QByteArray& message);
    void errorOccurred(const QString& errorMessage);

private slots:
    void onConnected();
    void onDisconnected();
    void onReadyRead();
    void onError(QAbstractSocket::SocketError socketError);

private:
    QTcpSocket* socket_;
    QByteArray buffer_;
};