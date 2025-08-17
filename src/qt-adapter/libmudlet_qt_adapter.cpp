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

#include "libmudlet_qt_adapter.h"
#include <QTimer>
#include <QVariantMap>
#include <QStringList>

// LibMudletQtAdapter implementation

LibMudletQtAdapter::LibMudletQtAdapter(QObject* parent)
    : QObject(parent)
    , mProcessTimer(new QTimer(this)) {
    
    connect(mProcessTimer, &QTimer::timeout, this, &LibMudletQtAdapter::onTimerTimeout);
}

LibMudletQtAdapter::~LibMudletQtAdapter() {
    shutdown();
}

bool LibMudletQtAdapter::initialize() {
    if (mInitialized) {
        return true;
    }
    
    mudlet::initialize();
    
    mCore = mudlet::Core::create();
    if (!mCore) {
        return false;
    }
    
    setupCallbacks();
    
    // Start processing timer (10ms intervals)
    mProcessTimer->start(10);
    
    mInitialized = true;
    return true;
}

void LibMudletQtAdapter::shutdown() {
    if (!mInitialized) {
        return;
    }
    
    mProcessTimer->stop();
    mCore.reset();
    mudlet::shutdown();
    mInitialized = false;
}

QString LibMudletQtAdapter::createSession(const QString& name) {
    if (!mCore) {
        return QString();
    }
    
    auto session = mCore->createSession(name.toStdString());
    if (session) {
        emit sessionCreated(name);
        return name;
    }
    
    return QString();
}

bool LibMudletQtAdapter::removeSession(const QString& name) {
    if (!mCore) {
        return false;
    }
    
    bool removed = mCore->removeSession(name.toStdString());
    if (removed) {
        emit sessionRemoved(name);
    }
    
    return removed;
}

QStringList LibMudletQtAdapter::getSessionNames() const {
    if (!mCore) {
        return QStringList();
    }
    
    auto names = mCore->getSessionNames();
    QStringList qnames;
    for (const auto& name : names) {
        qnames << QString::fromStdString(name);
    }
    
    return qnames;
}

void LibMudletQtAdapter::processEvents() {
    if (mCore) {
        mCore->processEvents();
    }
}

void LibMudletQtAdapter::onTimerTimeout() {
    processEvents();
}

void LibMudletQtAdapter::setupCallbacks() {
    if (!mCore) {
        return;
    }
    
    mCore->setEventCallback([this](const mudlet::Event& event) {
        convertEvent(event);
    });
}

void LibMudletQtAdapter::convertEvent(const mudlet::Event& event) {
    QVariantMap data;
    for (const auto& [key, value] : event.data) {
        data[QString::fromStdString(key)] = QString::fromStdString(value);
    }
    
    QString type;
    switch (event.type) {
        case mudlet::EventType::TextReceived:
            type = "textReceived";
            break;
        case mudlet::EventType::ConnectionStateChanged:
            type = "connectionStateChanged";
            break;
        case mudlet::EventType::TriggerFired:
            type = "triggerFired";
            break;
        case mudlet::EventType::TimerFired:
            type = "timerFired";
            break;
        case mudlet::EventType::AliasExecuted:
            type = "aliasExecuted";
            break;
        case mudlet::EventType::KeyPressed:
            type = "keyPressed";
            break;
        case mudlet::EventType::ScriptCompleted:
            type = "scriptCompleted";
            break;
        case mudlet::EventType::Error:
            type = "error";
            break;
    }
    
    emit eventReceived(type, data);
}

// LibMudletSessionAdapter implementation

LibMudletSessionAdapter::LibMudletSessionAdapter(std::shared_ptr<mudlet::Session> session, QObject* parent)
    : QObject(parent)
    , mSession(session) {
    
    setupCallbacks();
}

LibMudletSessionAdapter::~LibMudletSessionAdapter() = default;

QString LibMudletSessionAdapter::getName() const {
    if (!mSession) {
        return QString();
    }
    
    return QString::fromStdString(mSession->getName());
}

bool LibMudletSessionAdapter::isConnected() const {
    if (!mSession) {
        return false;
    }
    
    return mSession->isConnected();
}

bool LibMudletSessionAdapter::connectToServer(const QString& hostname, int port, bool ssl) {
    if (!mSession) {
        return false;
    }
    
    mudlet::ConnectionInfo info;
    info.hostname = hostname.toStdString();
    info.port = port;
    info.ssl = ssl;
    
    return mSession->connect(info, [this](bool connected, const std::string& error) {
        if (connected) {
            emit this->connected();
        } else {
            emit connectionError(QString::fromStdString(error));
        }
    });
}

void LibMudletSessionAdapter::disconnect() {
    if (mSession) {
        mSession->disconnect();
    }
}

void LibMudletSessionAdapter::send(const QString& text, bool echo) {
    if (mSession) {
        mSession->send(text.toStdString(), echo);
    }
}

int LibMudletSessionAdapter::addTrigger(const QString& pattern, const QString& script, bool enabled) {
    if (!mSession) {
        return -1;
    }
    
    mudlet::Trigger trigger;
    trigger.pattern = pattern.toStdString();
    trigger.script = script.toStdString();
    trigger.enabled = enabled;
    trigger.patternType = mudlet::PatternType::Substring;
    
    return mSession->getTriggerEngine().addTrigger(trigger);
}

bool LibMudletSessionAdapter::removeTrigger(int id) {
    if (!mSession) {
        return false;
    }
    
    return mSession->getTriggerEngine().removeTrigger(id);
}

bool LibMudletSessionAdapter::setTriggerEnabled(int id, bool enabled) {
    if (!mSession) {
        return false;
    }
    
    return mSession->getTriggerEngine().setTriggerEnabled(id, enabled);
}

int LibMudletSessionAdapter::addAlias(const QString& pattern, const QString& command, bool enabled) {
    if (!mSession) {
        return -1;
    }
    
    mudlet::Alias alias;
    alias.pattern = pattern.toStdString();
    alias.command = command.toStdString();
    alias.enabled = enabled;
    alias.patternType = mudlet::PatternType::Exact;
    
    return mSession->getAliasEngine().addAlias(alias);
}

bool LibMudletSessionAdapter::removeAlias(int id) {
    if (!mSession) {
        return false;
    }
    
    return mSession->getAliasEngine().removeAlias(id);
}

bool LibMudletSessionAdapter::setAliasEnabled(int id, bool enabled) {
    if (!mSession) {
        return false;
    }
    
    return mSession->getAliasEngine().setAliasEnabled(id, enabled);
}

int LibMudletSessionAdapter::addTimer(int intervalMs, const QString& script, bool enabled) {
    if (!mSession) {
        return -1;
    }
    
    mudlet::Timer timer;
    timer.intervalMs = intervalMs;
    timer.script = script.toStdString();
    timer.enabled = enabled;
    timer.autoStart = true;
    
    return mSession->getTimerEngine().addTimer(timer);
}

bool LibMudletSessionAdapter::removeTimer(int id) {
    if (!mSession) {
        return false;
    }
    
    return mSession->getTimerEngine().removeTimer(id);
}

bool LibMudletSessionAdapter::startTimer(int id) {
    if (!mSession) {
        return false;
    }
    
    return mSession->getTimerEngine().startTimer(id);
}

bool LibMudletSessionAdapter::stopTimer(int id) {
    if (!mSession) {
        return false;
    }
    
    return mSession->getTimerEngine().stopTimer(id);
}

void LibMudletSessionAdapter::onTextReceived(const mudlet::TextLine& line) {
    QString text = convertTextLine(line);
    emit textReceived(text);
}

void LibMudletSessionAdapter::onConnectionStatusChanged(mudlet::ConnectionState state) {
    switch (state) {
        case mudlet::ConnectionState::Connected:
            emit connected();
            break;
        case mudlet::ConnectionState::Disconnected:
            emit disconnected();
            break;
        case mudlet::ConnectionState::Error:
            emit connectionError("Connection error");
            break;
        default:
            break;
    }
}

void LibMudletSessionAdapter::setupCallbacks() {
    if (!mSession) {
        return;
    }
    
    mSession->setTextReceivedCallback([this](const mudlet::TextLine& line) {
        onTextReceived(line);
    });
    
    mSession->setConnectionStatusCallback([this](mudlet::ConnectionState state) {
        onConnectionStatusChanged(state);
    });
    
    // Setup engine callbacks
    mSession->getTriggerEngine().setTriggerCallback([this](int triggerId, const mudlet::MatchInfo& match) {
        emit triggerFired(triggerId, QString::fromStdString(match.fullMatch));
    });
    
    mSession->getAliasEngine().setAliasCallback([this](int aliasId, const mudlet::MatchInfo& match, const std::string& expansion) {
        emit aliasExecuted(aliasId, QString::fromStdString(match.fullMatch), QString::fromStdString(expansion));
    });
    
    mSession->getTimerEngine().setTimerCallback([this](int timerId) {
        emit timerFired(timerId);
    });
}

QString LibMudletSessionAdapter::convertTextLine(const mudlet::TextLine& line) {
    return QString::fromStdString(line.text);
}

#include "libmudlet_qt_adapter.moc"