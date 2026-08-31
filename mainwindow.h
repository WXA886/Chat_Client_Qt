#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "qt_client.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onSendClicked();
    void onLoginClicked();
    void onRegisterClicked();
    void onLogoutClicked();
    void onLoginResult(bool ok, const QString &msg);
    void onRegisterResult(bool ok, const QString &msg);

private:
    Ui::MainWindow *ui;
    QtClient *m_client;
};

#endif
