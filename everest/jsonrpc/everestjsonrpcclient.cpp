#include "everestjsonrpcclient.h"
#include "extern-plugininfo.h"

EverestJsonRpcClient::EverestJsonRpcClient(QObject *parent)
    : QObject{parent}
{
    m_webSocket = new QWebSocket("nymea-client", QWebSocketProtocol::Version13, this);

    connect(m_webSocket, &QWebSocket::disconnected, this, &EverestJsonRpcClient::onDisconnected);
    connect(m_webSocket, &QWebSocket::textMessageReceived, this, &EverestJsonRpcClient::onTextMessageReceived);
    connect(m_webSocket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(onError(QAbstractSocket::SocketError)));
    connect(m_webSocket, SIGNAL(stateChanged(QAbstractSocket::SocketState)), this, SLOT(onStateChanged(QAbstractSocket::SocketState)));
}

EverestJsonRpcClient::~EverestJsonRpcClient()
{
    disconnectServer();
}

void EverestJsonRpcClient::sendData(const QByteArray &data)
{
    m_webSocket->sendTextMessage(QString::fromUtf8(data));
}

void EverestJsonRpcClient::connectServer(const QUrl &serverUrl)
{
    if (m_connected) {
        m_connected = false;
        emit connectedChanged(m_connected);
        m_webSocket->close();
    }

    m_serverUrl = serverUrl;

    qCDebug(dcEverest()) << "Connecting to" << m_serverUrl.toString();
    m_webSocket->open(m_serverUrl);
}

void EverestJsonRpcClient::disconnectServer()
{
    qCDebug(dcEverest()) << "Disconnecting from" << m_serverUrl.toString();
    m_webSocket->close();
}

void EverestJsonRpcClient::onDisconnected()
{
    qCDebug(dcEverest()) << "Disconnected from" << m_webSocket->requestUrl().toString() << m_webSocket->closeReason();

    if (m_connected) {
        m_connected = false;
        emit connectedChanged(m_connected);
    }
}

void EverestJsonRpcClient::onError(QAbstractSocket::SocketError error)
{
    qCDebug(dcEverest()) << "Socket error occurred" << error << m_webSocket->errorString();
}

void EverestJsonRpcClient::onStateChanged(QAbstractSocket::SocketState state)
{
    qCDebug(dcEverest()) << "Socket state changed" << state;

    switch (state) {
    case QAbstractSocket::ConnectedState:
        qCDebug(dcEverest()) << "Connected with" << m_webSocket->requestUrl().toString();
        if (!m_connected) {
            m_connected = true;
            emit connectedChanged(m_connected);
        }
        break;
    default:
        if (m_connected) {
            m_connected = false;
            emit connectedChanged(m_connected);
        }
        break;
    }
}

void EverestJsonRpcClient::onTextMessageReceived(const QString &message)
{
    emit dataReceived(message.toUtf8());
}
