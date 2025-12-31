#ifndef MUDLET_NOTESMANAGER_H
#define MUDLET_NOTESMANAGER_H

/***************************************************************************
 *   Copyright (C) 2008-2009 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2025 by Mudlet Development Team                         *
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


#include <QDateTime>
#include <QMap>
#include <QObject>
#include <QPointer>
#include <QList>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

class Host;

class NotesManager : public QObject
{
    Q_OBJECT

public:
    struct NoteTab {
        QString name;
        QString content;
        QDateTime lastModified;
        bool isDirty;
    };

    explicit NotesManager(Host* pH);
    ~NotesManager();

    // Tab management
    QString addTab(const QString& tabName = QString());
    void removeTab(const QString& tabId);
    void renameTab(const QString& tabId, const QString& newName);
    QList<NoteTab> getAllTabs() const;

    // Content management
    void setTabContent(const QString& tabId, const QString& content);
    QString getTabContent(const QString& tabId) const;

    // Persistence
    void save();
    void restore();
    void saveSettings();
    void restoreSettings();

    void setNeedToSave(bool needToSave) { mNeedToSave = needToSave; }
    bool getNeedToSave() const { return mNeedToSave; }

    const QMap<QString, NoteTab>& getTabsMap() const { return mTabs; }

signals:
    void tabAdded(const QString& tabId, const QString& tabName);
    void tabRemoved(const QString& tabId);
    void tabRenamed(const QString& tabId, const QString& newName);
    void contentChanged(const QString& tabId);

private:
    QPointer<Host> mpHost;
    QMap<QString, NoteTab> mTabs;
    bool mNeedToSave = false;

    void initializeDefaultTab();
    QString generateTabId();
    void migrateFromOldFormat();
};

#endif // MUDLET_NOTESMANAGER_H

