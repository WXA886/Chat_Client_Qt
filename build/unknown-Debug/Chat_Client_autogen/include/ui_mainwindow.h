/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 5.15.13
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QWidget *centralwidget;
    QHBoxLayout *horizontalLayout;
    QWidget *chatWidget;
    QVBoxLayout *verticalLayout_chat;
    QTextEdit *textEdit_display;
    QHBoxLayout *horizontalLayout_input;
    QLineEdit *lineEdit_input;
    QPushButton *pushButton_send;
    QWidget *sideWidget;
    QVBoxLayout *verticalLayout_side;
    QLineEdit *lineEdit_username;
    QLineEdit *lineEdit_password;
    QHBoxLayout *horizontalLayout_auth;
    QPushButton *pushButton_login;
    QPushButton *pushButton_register;
    QPushButton *pushButton_logout;
    QListWidget *listWidget_users;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName(QString::fromUtf8("MainWindow"));
        MainWindow->resize(800, 600);
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName(QString::fromUtf8("centralwidget"));
        horizontalLayout = new QHBoxLayout(centralwidget);
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        chatWidget = new QWidget(centralwidget);
        chatWidget->setObjectName(QString::fromUtf8("chatWidget"));
        verticalLayout_chat = new QVBoxLayout(chatWidget);
        verticalLayout_chat->setObjectName(QString::fromUtf8("verticalLayout_chat"));
        textEdit_display = new QTextEdit(chatWidget);
        textEdit_display->setObjectName(QString::fromUtf8("textEdit_display"));

        verticalLayout_chat->addWidget(textEdit_display);

        horizontalLayout_input = new QHBoxLayout();
        horizontalLayout_input->setObjectName(QString::fromUtf8("horizontalLayout_input"));
        lineEdit_input = new QLineEdit(chatWidget);
        lineEdit_input->setObjectName(QString::fromUtf8("lineEdit_input"));

        horizontalLayout_input->addWidget(lineEdit_input);

        pushButton_send = new QPushButton(chatWidget);
        pushButton_send->setObjectName(QString::fromUtf8("pushButton_send"));

        horizontalLayout_input->addWidget(pushButton_send);


        verticalLayout_chat->addLayout(horizontalLayout_input);


        horizontalLayout->addWidget(chatWidget);

        sideWidget = new QWidget(centralwidget);
        sideWidget->setObjectName(QString::fromUtf8("sideWidget"));
        verticalLayout_side = new QVBoxLayout(sideWidget);
        verticalLayout_side->setObjectName(QString::fromUtf8("verticalLayout_side"));
        lineEdit_username = new QLineEdit(sideWidget);
        lineEdit_username->setObjectName(QString::fromUtf8("lineEdit_username"));

        verticalLayout_side->addWidget(lineEdit_username);

        lineEdit_password = new QLineEdit(sideWidget);
        lineEdit_password->setObjectName(QString::fromUtf8("lineEdit_password"));
        lineEdit_password->setEchoMode(QLineEdit::Password);

        verticalLayout_side->addWidget(lineEdit_password);

        horizontalLayout_auth = new QHBoxLayout();
        horizontalLayout_auth->setObjectName(QString::fromUtf8("horizontalLayout_auth"));
        pushButton_login = new QPushButton(sideWidget);
        pushButton_login->setObjectName(QString::fromUtf8("pushButton_login"));

        horizontalLayout_auth->addWidget(pushButton_login);

        pushButton_register = new QPushButton(sideWidget);
        pushButton_register->setObjectName(QString::fromUtf8("pushButton_register"));

        horizontalLayout_auth->addWidget(pushButton_register);


        verticalLayout_side->addLayout(horizontalLayout_auth);

        pushButton_logout = new QPushButton(sideWidget);
        pushButton_logout->setObjectName(QString::fromUtf8("pushButton_logout"));

        verticalLayout_side->addWidget(pushButton_logout);

        listWidget_users = new QListWidget(sideWidget);
        listWidget_users->setObjectName(QString::fromUtf8("listWidget_users"));

        verticalLayout_side->addWidget(listWidget_users);


        horizontalLayout->addWidget(sideWidget);

        MainWindow->setCentralWidget(centralwidget);

        retranslateUi(MainWindow);

        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QCoreApplication::translate("MainWindow", "Qt \350\201\212\345\244\251\345\256\242\346\210\267\347\253\257", nullptr));
        pushButton_send->setText(QCoreApplication::translate("MainWindow", "\345\217\221\351\200\201", nullptr));
        lineEdit_username->setPlaceholderText(QCoreApplication::translate("MainWindow", "\347\224\250\346\210\267\345\220\215", nullptr));
        lineEdit_password->setPlaceholderText(QCoreApplication::translate("MainWindow", "\345\257\206\347\240\201", nullptr));
        pushButton_login->setText(QCoreApplication::translate("MainWindow", "\347\231\273\345\275\225", nullptr));
        pushButton_register->setText(QCoreApplication::translate("MainWindow", "\346\263\250\345\206\214", nullptr));
        pushButton_logout->setText(QCoreApplication::translate("MainWindow", "\347\231\273\345\207\272", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
