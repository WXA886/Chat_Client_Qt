#include "qt_client.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

QtClient::QtClient(QObject *parent)
    : QObject(parent)
    , m_socket(new QTcpSocket(this))
    , m_heartbeatTimer(new QTimer(this))
    , m_display(nullptr)
    , m_userList(nullptr)
{
    init_recv_buffer(&m_recvBuf);

    connect(m_socket, &QTcpSocket::connected, this, &QtClient::onConnected);
    connect(m_socket, &QTcpSocket::disconnected, this, &QtClient::onDisconnected);
    connect(m_socket, &QTcpSocket::readyRead, this, &QtClient::onReadyRead);
    connect(m_socket, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::errorOccurred),
            this, &QtClient::onError);

    // 心跳定时器：每 10 秒发送一次
    connect(m_heartbeatTimer, &QTimer::timeout, this, &QtClient::sendHeartbeat);
    m_heartbeatTimer->setInterval(10000);
}

QtClient::~QtClient()
{
    if (m_socket->state() == QAbstractSocket::ConnectedState) {
        m_socket->disconnectFromHost();
    }
    m_heartbeatTimer->stop();
}

void QtClient::connectToServer(const QString &ip, int port)
{
    m_socket->connectToHost(ip, port);
}

void QtClient::setMessageDisplay(QTextEdit *display)
{
    m_display = display;
}

void QtClient::setUserList(QListWidget *list)
{
    m_userList = list;
}

void QtClient::onConnected()
{
    appendMessage("系统", "已连接到服务器", true);
    emit connected();
    // 连接成功后启动心跳
    m_heartbeatTimer->start();
}

void QtClient::onDisconnected()
{
    appendMessage("系统", "与服务器断开连接", true);
    emit disconnected();
    // 断开后停止心跳
    m_heartbeatTimer->stop();
}

void QtClient::onError(QAbstractSocket::SocketError error)
{
    Q_UNUSED(error)
    appendMessage("系统", "网络错误: " + m_socket->errorString(), true);
    m_heartbeatTimer->stop();
}

void QtClient::onReadyRead()
{
    QByteArray data = m_socket->readAll();

    if (m_recvBuf.data_len + (size_t)data.size() > RECV_BUFFER_SIZE) {
        qWarning() << "接收缓冲区溢出";
        return;
    }

    memcpy(m_recvBuf.buffer + m_recvBuf.data_len,
           data.constData(),
           (size_t)data.size());
    m_recvBuf.data_len += (size_t)data.size();

    char *msg;
    while ((msg = extract_message(&m_recvBuf)) != NULL) {
        processMessage(QString::fromUtf8(msg));
        free(msg);
    }
}

void QtClient::processMessage(const QString &jsonMsg)
{
    QJsonDocument doc = QJsonDocument::fromJson(jsonMsg.toUtf8());
    if (!doc.isObject()) return;

    QJsonObject obj = doc.object();
    QString type = obj["type"].toString();

    // 心跳确认 -- 静默处理，不显示
    if (type == "heartbeat_ack") {
        return;
    }

    if (type == "system") {
        QString msg = obj["message"].toString();
        QString username = obj["username"].toString();
        appendMessage(username.isEmpty() ? "系统" : username, msg, true);
        return;
    }

    if (type == "chat") {
        QString from = obj["from"].toString();
        QString content = obj["content"].toString();
        appendMessage(from, content);
        return;
    }

    if (type == "private_chat") {
        QString from = obj["from"].toString();
        QString content = obj["content"].toString();
        appendMessage("🔒 " + from + " (私聊)", content);
        return;
    }

    if (type == "register_result" || type == "login_result") {
        bool ok = obj["ok"].toBool();
        QString msg = obj["message"].toString();

        if (type == "register_result") {
            emit registerResult(ok, msg);
        } else {
            emit loginResult(ok, msg);
            if (ok) {
                appendMessage("系统", "登录成功", true);
            }
        }
        return;
    }

    // 其他消息直接显示
    appendMessage("服务器", jsonMsg, true);
}

void QtClient::appendMessage(const QString &from, const QString &content, bool isSystem)
{
    if (!m_display) return;

    QString line;
    if (isSystem) {
        line = QString("[系统] %1").arg(content);
    } else {
        line = QString("[%1] %2").arg(from, content);
    }

    m_display->append(line);
}

void QtClient::sendJson(const QJsonObject &json)
{
    QByteArray data = QJsonDocument(json).toJson(QJsonDocument::Compact);
    int result = send_message(m_socket->socketDescriptor(), data.constData());
    if (result < 0) {
        appendMessage("系统", "发送失败: " + m_socket->errorString(), true);
    }
}

// 发送心跳
void QtClient::sendHeartbeat()
{
    if (m_socket->state() == QAbstractSocket::ConnectedState) {
        QJsonObject json;
        json["type"] = "heartbeat";
        QByteArray data = QJsonDocument(json).toJson(QJsonDocument::Compact);
        send_message(m_socket->socketDescriptor(), data.constData());
    } else {
        // 如果已经断开，停止心跳
        m_heartbeatTimer->stop();
    }
}

void QtClient::sendChatMessage(const QString &content)
{
    QJsonObject json;
    json["type"] = "chat";
    json["content"] = content;
    sendJson(json);
}

void QtClient::sendPrivateMessage(const QString &to, const QString &content)
{
    QJsonObject json;
    json["type"] = "private_chat";
    json["to"] = to;
    json["content"] = content;
    sendJson(json);
}

void QtClient::sendLogin(const QString &username, const QString &password)
{
    QJsonObject json;
    json["type"] = "login";
    json["username"] = username;
    json["password"] = password;
    sendJson(json);
}

void QtClient::sendRegister(const QString &username, const QString &password)
{
    QJsonObject json;
    json["type"] = "register";
    json["username"] = username;
    json["password"] = password;
    sendJson(json);
}

void QtClient::sendLogout()
{
    QJsonObject json;
    json["type"] = "logout";
    sendJson(json);
}