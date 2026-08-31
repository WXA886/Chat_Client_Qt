#ifndef QT_CLIENT_H
#define QT_CLIENT_H

#include <QObject>
#include <QTcpSocket>
#include <QTextEdit>
#include <QListWidget>
#include <QJsonObject>
#include <QTimer>
#include "include/protocol.h"

class QtClient : public QObject
{
    Q_OBJECT

public:
    explicit QtClient(QObject *parent = nullptr);
    ~QtClient();

    // 连接服务器
    void connectToServer(const QString &ip, int port);

    // 发送消息
    void sendChatMessage(const QString &content);
    void sendPrivateMessage(const QString &to, const QString &content);
    void sendLogin(const QString &username, const QString &password);
    void sendRegister(const QString &username, const QString &password);
    void sendLogout();

    // 关联 UI 组件
    void setMessageDisplay(QTextEdit *display);
    void setUserList(QListWidget *list);

    bool isConnected() const { return m_socket->state() == QAbstractSocket::ConnectedState; }

signals:
    void connected();
    void disconnected();
    void loginResult(bool ok, const QString &msg);
    void registerResult(bool ok, const QString &msg);

private slots:
    void onReadyRead();
    void onConnected();
    void onDisconnected();
    void onError(QAbstractSocket::SocketError error);
    void sendHeartbeat();  // 心跳槽函数

private:
    QTcpSocket *m_socket;
    recv_buffer_t m_recvBuf;
    QString m_username;
    QTimer *m_heartbeatTimer;  // 心跳定时器

    QTextEdit *m_display;
    QListWidget *m_userList;

    void processMessage(const QString &jsonMsg);
    void sendJson(const QJsonObject &json);
    void appendMessage(const QString &from, const QString &content, bool isSystem = false);
};

#endif