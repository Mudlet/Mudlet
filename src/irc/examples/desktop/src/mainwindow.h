/*
* Copyright (C) 2008-2012 J-P Nurmi <jpnurmi@gmail.com>
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*/

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "trayicon.h"

class MultiSessionTabWidget;
class SessionTreeWidget;
class SessionTreeItem;
struct ConnectionInfo;
class MessageView;
class IrcMessage;
class QtDockTile;
struct Settings;
class Session;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = 0);
    ~MainWindow();

    QSize sizeHint() const;

public slots:
    void connectTo(const QString& host = QString(), quint16 port = 6667,
                   const QString& nick = QString(), const QString& password = QString());
    void connectTo(const ConnectionInfo& connection);
    void connectToImpl(const ConnectionInfo& connection);

protected:
    void closeEvent(QCloseEvent* event);
    void changeEvent(QEvent* event);

private slots:
    void initialize();
    void editSession(Session* session);
    void applySettings(const Settings& settings);
    void trayIconActivated(QSystemTrayIcon::ActivationReason reason);
    void alert(MessageView* view, IrcMessage* message);
    void highlight(MessageView* view, IrcMessage* message);
    void viewAdded(MessageView* view);
    void viewRemoved(MessageView* view);
    void viewRenamed(const QString& from, const QString& to);
    void viewActivated(MessageView* view);
    void currentTreeItemChanged(Session* session, const QString& view);
    void menuRequested(SessionTreeItem* item, const QPoint& pos);
    void splitterChanged(const QByteArray& state);
    void sessionAdded(Session* session);
    void sessionRemoved(Session* session);
    void updateSession(Session* session = 0);
    void addView();

private:
    void createTree();

    SessionTreeWidget* treeWidget;
    MultiSessionTabWidget* tabWidget;
    TrayIcon* trayIcon;
    QtDockTile* dockTile;
};

#endif // MAINWINDOW_H
