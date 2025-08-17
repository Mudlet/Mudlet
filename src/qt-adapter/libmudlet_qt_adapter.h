/***************************************************************************
 *   Copyright (C) 2025 by Rishi Mondal - mavrickrishi@gmail.com          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef LIBMUDLET_QT_ADAPTER_H
#define LIBMUDLET_QT_ADAPTER_H

#include <QObject>
#include <QString>
#include <QTimer>
#include <memory>
#include "mudlet/libmudlet.h"

class QTimer;

/**
 * @brief Qt adapter for libmudlet Core
 * 
 * Bridges libmudlet callbacks to Qt signals and provides
 * Qt-friendly interfaces for the existing Mudlet codebase.
 */
class LibMudletQtAdapter : public QObject {
    Q_OBJECT

public:
    explicit LibMudletQtAdapter(QObject* parent = nullptr);
    ~LibMudletQtAdapter();

    // Core management
    bool initialize();
    void shutdown();
    
    // Session management
    QString createSession(const QString& name);
    bool removeSession(const QString& name);
    QStringList getSessionNames() const;
    
    // Process events (call regularly from Qt event loop)
    void processEvents();

signals:
    // Core events
    void sessionCreated(const QString& sessionName);
    void sessionRemoved(const QString& sessionName);
    void eventReceived(const QString& type, const QVariantMap& data);

public slots:
    void onTimerTimeout();

private:
    std::unique_ptr<mudlet::Core> mCore;
    QTimer* mProcessTimer;
    bool mInitialized = false;
    
    void setupCallbacks();
    void convertEvent(const mudlet::Event& event);
};

/**
 * @brief Qt adapter for libmudlet Session
 * 
 * Provides Qt signals/slots interface for session operations
 * while delegating to libmudlet Session implementation.
 */
class LibMudletSessionAdapter : public QObject {
    Q_OBJECT

public:
    explicit LibMudletSessionAdapter(std::shared_ptr<mudlet::Session> session, QObject* parent = nullptr);
    ~LibMudletSessionAdapter();

    QString getName() const;
    bool isConnected() const;
    
    // Connection management
    bool connectToServer(const QString& hostname, int port, bool ssl = false);
    void disconnect();
    
    // Text operations
    void send(const QString& text, bool echo = true);
    
    // Engine access
    int addTrigger(const QString& pattern, const QString& script, bool enabled = true);
    bool removeTrigger(int id);
    bool setTriggerEnabled(int id, bool enabled);
    
    int addAlias(const QString& pattern, const QString& command, bool enabled = true);
    bool removeAlias(int id);
    bool setAliasEnabled(int id, bool enabled);
    
    int addTimer(int intervalMs, const QString& script, bool enabled = true);
    bool removeTimer(int id);
    bool startTimer(int id);
    bool stopTimer(int id);

signals:
    // Connection events
    void connected();
    void disconnected();
    void connectionError(const QString& error);
    
    // Text events
    void textReceived(const QString& text);
    
    // Engine events
    void triggerFired(int triggerId, const QString& match);
    void aliasExecuted(int aliasId, const QString& command, const QString& expansion);
    void timerFired(int timerId);

private slots:
    void onTextReceived(const mudlet::TextLine& line);
    void onConnectionStatusChanged(mudlet::ConnectionState state);

private:
    std::shared_ptr<mudlet::Session> mSession;
    
    void setupCallbacks();
    QString convertTextLine(const mudlet::TextLine& line);
};

#endif // LIBMUDLET_QT_ADAPTER_H