#pragma once

#include <QJsonObject>
#include <QObject>
#include <QTcpSocket>

/* SimClient — connects to rr-feedback --serve via TCP and exchanges JSON Lines.
   Sends plain text commands (step\n, reset\n, status\n).
   Emits snapshotReceived for each complete JSON line from the server. */
class SimClient : public QObject {
    Q_OBJECT

  public:
    explicit SimClient(QObject *parent = nullptr);

    void connectToServer(const QString &host, quint16 port);

    void step();
    void reset();
    void status();

  signals:
    void connected();
    void disconnected();
    void snapshotReceived(const QJsonObject &snapshot, const QString &raw);
    void errorOccurred(const QString &message);

  private slots:
    void onConnected();
    void onDisconnected();
    void onReadyRead();
    void onErrorOccurred(QAbstractSocket::SocketError error);

  private:
    void sendLine(const char *cmd);

    QTcpSocket *m_socket;
    QByteArray m_buffer;
};
