/*
 * Copyright (C) 2008-2016 The Communi Project
 *
 * This example is free, and not covered by the BSD license. There is no
 * restriction applied to their modification, redistribution, using and so on.
 * You can study them, modify them, use them in your own program - either
 * completely or partially.
 */

#ifndef IRCCLIENT_H
#define IRCCLIENT_H

#include <QSplitter>
#include <QHash>

class IrcBuffer;
class IrcMessage;
class IrcUserModel;
class IrcCompleter;
class IrcConnection;
class IrcBufferModel;
class IrcCommandParser;

QT_FORWARD_DECLARE_CLASS(QLineEdit)
QT_FORWARD_DECLARE_CLASS(QListView)
QT_FORWARD_DECLARE_CLASS(QTextEdit)
QT_FORWARD_DECLARE_CLASS(QModelIndex)
QT_FORWARD_DECLARE_CLASS(QTextDocument)

class IrcClient : public QSplitter
{
    Q_OBJECT

public:
    IrcClient(QWidget* parent = 0);
    ~IrcClient();

private slots:
    void onConnected();
    void onConnecting();
    void onDisconnected();

    void onTextEdited();
    void onTextEntered();

    void onCompletion();
    void onCompleted(const QString& text, int cursor);

    void onBufferAdded(IrcBuffer* buffer);
    void onBufferRemoved(IrcBuffer* buffer);

    void onBufferActivated(const QModelIndex& index);
    void onUserActivated(const QModelIndex& index);

    void receiveMessage(IrcMessage* message);

private:
    void createLayout();
    void createCompleter();
    void createParser();
    void createUserList();
    void createBufferList();
    void createConnection();

    QLineEdit* lineEdit;
    QTextEdit* textEdit;
    QListView* userList;
    QListView* bufferList;

    IrcCompleter* completer;
    IrcCommandParser* parser;
    IrcConnection* connection;
    IrcBufferModel* bufferModel;
    QHash<IrcBuffer*, IrcUserModel*> userModels;
    QHash<IrcBuffer*, QTextDocument*> documents;
};

#endif // IRCCLIENT_H
