// /src/network/NetworkClient.h
#pragma once

// Include compatibility header first
#include "../QtCompat.h"
#include <QtCore/QObject>
#include <QtCore/QtGlobal>
#include <QtNetwork/QTcpSocket>
#include <QtNetwork/QHostAddress>
#include <QtNetwork/QAbstractSocket>
#include <QtNetwork/QNetworkInterface>
#include <QtCore/QString>
#include <QtCore/QByteArray>

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
    void rawDataReceived(const QByteArray& data);  // Raw network data
    void moveReceived(const QString& from, const QString& to);
    void errorOccurred(const QString& errorMessage);

public slots:
    void processNetworkData(const QByteArray& data);

private slots:
    void onConnected();
    void onDisconnected();
    void onReadyRead();
    void onError(QAbstractSocket::SocketError socketError);

private:
    QTcpSocket* socket_;
    QByteArray buffer_;
};