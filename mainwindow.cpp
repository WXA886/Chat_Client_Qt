#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <QDebug>   // 👈 添加这行

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_client(new QtClient(this))
{
    ui->setupUi(this);

    // 关联客户端 UI 组件
    m_client->setMessageDisplay(ui->textEdit_display);
    m_client->setUserList(ui->listWidget_users);

    // 连接信号槽
    connect(ui->pushButton_send, &QPushButton::clicked, this, &MainWindow::onSendClicked);
    connect(ui->lineEdit_input, &QLineEdit::returnPressed, this, &MainWindow::onSendClicked);
    connect(ui->pushButton_login, &QPushButton::clicked, this, &MainWindow::onLoginClicked);
    connect(ui->pushButton_register, &QPushButton::clicked, this, &MainWindow::onRegisterClicked);
    connect(ui->pushButton_logout, &QPushButton::clicked, this, &MainWindow::onLogoutClicked);

    connect(m_client, &QtClient::loginResult, this, &MainWindow::onLoginResult);
    connect(m_client, &QtClient::registerResult, this, &MainWindow::onRegisterResult);

    // 👇 添加调试：检查信号槽连接是否成功
    bool ok1 = connect(ui->pushButton_send, &QPushButton::clicked, this, &MainWindow::onSendClicked);
    qDebug() << "pushButton_send 连接状态:" << ok1;

    // 自动连接服务器
    m_client->connectToServer("43.138.249.232", 9527);

    setWindowTitle("Qt 聊天客户端");
    ui->textEdit_display->append("=== 欢迎使用 Qt 聊天客户端 ===");
    ui->textEdit_display->append("请先注册或登录，然后开始聊天");
    ui->textEdit_display->append("私聊格式: /to 用户名 消息内容\n");
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::onSendClicked()
{
    // 👇 添加调试：确认函数被调用
    qDebug() << "onSendClicked 被调用了！";

    QString text = ui->lineEdit_input->text().trimmed();
    qDebug() << "输入内容:" << text;

    if (text.isEmpty()) {
        qDebug() << "输入为空，返回";
        return;
    }

    // 检测私聊命令
    if (text.startsWith("/to ")) {
        QString rest = text.mid(4).trimmed();
        int spaceIdx = rest.indexOf(' ');
        if (spaceIdx > 0) {
            QString to = rest.left(spaceIdx);
            QString content = rest.mid(spaceIdx + 1);
            if (!content.isEmpty()) {
                m_client->sendPrivateMessage(to, content);
                // 🟢 显示自己发送的私聊消息
                ui->textEdit_display->append(QString("🧑 [我 → %1] %2").arg(to, content));
                qDebug() << "发送私聊: to=" << to << "content=" << content;
            }
        } else {
            ui->textEdit_display->append("[错误] 格式: /to 用户名 消息内容");
        }
    } else {
        m_client->sendChatMessage(text);
        // 🟢 显示自己发送的群聊消息
        ui->textEdit_display->append(QString("🧑 [我] %1").arg(text));
        qDebug() << "发送群聊: " << text;
    }

    ui->lineEdit_input->clear();
}

void MainWindow::onLoginClicked()
{
    QString username = ui->lineEdit_username->text().trimmed();
    QString password = ui->lineEdit_password->text().trimmed();
    if (username.isEmpty() || password.isEmpty()) {
        QMessageBox::warning(this, "提示", "请输入用户名和密码");
        return;
    }
    m_client->sendLogin(username, password);
}

void MainWindow::onRegisterClicked()
{
    QString username = ui->lineEdit_username->text().trimmed();
    QString password = ui->lineEdit_password->text().trimmed();
    if (username.isEmpty() || password.isEmpty()) {
        QMessageBox::warning(this, "提示", "请输入用户名和密码");
        return;
    }
    m_client->sendRegister(username, password);
}

void MainWindow::onLogoutClicked()
{
    m_client->sendLogout();
    ui->textEdit_display->append("[系统] 已登出");
    setWindowTitle("Qt 聊天客户端");
}

void MainWindow::onLoginResult(bool ok, const QString &msg)
{
    if (ok) {
        QMessageBox::information(this, "登录", "登录成功");
        setWindowTitle("Qt 聊天客户端 - " + ui->lineEdit_username->text());
    } else {
        QMessageBox::warning(this, "登录失败", msg);
    }
}

void MainWindow::onRegisterResult(bool ok, const QString &msg)
{
    if (ok) {
        QMessageBox::information(this, "注册", "注册成功，请登录");
    } else {
        QMessageBox::warning(this, "注册失败", msg);
    }
}