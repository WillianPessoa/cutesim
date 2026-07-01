#include "SimClient.h"

#include <QJsonDocument>
#include <QJsonParseError>

SimClient::SimClient(QObject *parent)
    : QObject(parent)
    , m_socket(new QTcpSocket(this))
{
    connect(m_socket, &QTcpSocket::connected,    this, &SimClient::onConnected);
    connect(m_socket, &QTcpSocket::disconnected, this, &SimClient::onDisconnected);
    connect(m_socket, &QTcpSocket::readyRead,    this, &SimClient::onReadyRead);
    connect(m_socket, &QAbstractSocket::errorOccurred,
            this,     &SimClient::onErrorOccurred);
}

void SimClient::connectToServer(const QString &host, quint16 port)
{
    m_socket->connectToHost(host, port);
}

void SimClient::sendLine(const char *cmd)
{
    QByteArray data(cmd);
    data.append('\n');
    m_socket->write(data);
}

void SimClient::step()   { sendLine("step");   }
void SimClient::reset()  { sendLine("reset");  }
void SimClient::status() { sendLine("status"); }

void SimClient::onConnected()    { emit connected(); }
void SimClient::onDisconnected() { emit disconnected(); }

void SimClient::onReadyRead()
{
    m_buffer.append(m_socket->readAll());

    while (true) {
        int idx = m_buffer.indexOf('\n');
        if (idx < 0) break;

        QByteArray line = m_buffer.left(idx);
        m_buffer.remove(0, idx + 1);

        if (line.isEmpty()) continue;

        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(line, &err);
        if (err.error != QJsonParseError::NoError) {
            emit errorOccurred("JSON parse error: " + err.errorString());
            continue;
        }

        emit snapshotReceived(doc.object(), QString::fromUtf8(line));
    }
}

void SimClient::onErrorOccurred(QAbstractSocket::SocketError)
{
    emit errorOccurred(m_socket->errorString());
}
