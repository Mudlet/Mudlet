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


#include "NotesManager.h"
#include "Host.h"
#include "mudlet.h"

#include <QDir>
#include <QFile>
#include <QSaveFile>
#include <QSettings>
#include <QTextStream>
#include <QUuid>
#include <QStringConverter>

using namespace std::chrono;

const QString local8BitEncodedNotesFileName{qsl("notes.txt")};
const QString utf8EncodedNotesFileName{qsl("notes_utf8.txt")};
const QString jsonEncodedNotesFileName{qsl("notes.json")};

NotesManager::NotesManager(Host* pH)
: mpHost(pH)
{
    restore();
}

NotesManager::~NotesManager()
{
    if (mpHost && mNeedToSave) {
        save();
    }
}

QString NotesManager::addTab(const QString& tabName)
{
    QString finalName = tabName.isEmpty() ? tr("New Tab") : tabName;

    int counter = 1;
    QString uniqueName = finalName;
    while (std::any_of(mTabs.cbegin(), mTabs.cend(), [&uniqueName](const NoteTab& tab) { return tab.name == uniqueName; })) {
        uniqueName = qsl("%1 %2").arg(finalName).arg(counter++);
    }

    const QString tabId = generateTabId();
    NoteTab tab;
    tab.name = uniqueName;
    tab.content = QString();
    tab.lastModified = QDateTime::currentDateTimeUtc();
    tab.isDirty = true;

    mTabs.insert(tabId, tab);
    mNeedToSave = true;

    emit tabAdded(tabId, uniqueName);
    return tabId;
}

void NotesManager::removeTab(const QString& tabId)
{
    if (!mTabs.contains(tabId)) {
        return;
    }

    mTabs.remove(tabId);
    mNeedToSave = true;

    emit tabRemoved(tabId);
}

void NotesManager::renameTab(const QString& tabId, const QString& newName)
{
    if (!mTabs.contains(tabId)) {
        return;
    }

    mTabs[tabId].name = newName;
    mTabs[tabId].lastModified = QDateTime::currentDateTimeUtc();
    mTabs[tabId].isDirty = true;
    mNeedToSave = true;

    emit tabRenamed(tabId, newName);
}

QList<NotesManager::NoteTab> NotesManager::getAllTabs() const
{
    return mTabs.values();
}

void NotesManager::setTabContent(const QString& tabId, const QString& content)
{
    if (!mTabs.contains(tabId)) {
        return;
    }

    if (mTabs[tabId].content != content) {
        mTabs[tabId].content = content;
        mTabs[tabId].lastModified = QDateTime::currentDateTimeUtc();
        mTabs[tabId].isDirty = true;
        mNeedToSave = true;

        emit contentChanged(tabId);
    }
}

QString NotesManager::getTabContent(const QString& tabId) const
{
    if (!mTabs.contains(tabId)) {
        return QString();
    }

    return mTabs[tabId].content;
}

void NotesManager::save()
{
    if (!mpHost) {
        return;
    }

    const QString directoryFile = mudlet::getMudletPath(enums::profileHomePath, mpHost->getName());
    const QString fileName = mudlet::getMudletPath(enums::profileDataItemPath, mpHost->getName(), jsonEncodedNotesFileName);
    const QDir dirFile;
    if (!dirFile.exists(directoryFile)) {
        dirFile.mkpath(directoryFile);
    }

    QJsonObject rootObject;
    rootObject[qsl("version")] = 1;

    QJsonArray tabsArray;
    for (auto it = mTabs.constBegin(); it != mTabs.constEnd(); ++it) {
        QJsonObject tabObject;
        tabObject[qsl("id")] = it.key();
        tabObject[qsl("name")] = it.value().name;
        tabObject[qsl("content")] = it.value().content;
        tabObject[qsl("lastModified")] = it.value().lastModified.toString(Qt::ISODate);
        tabsArray.append(tabObject);
    }
    rootObject[qsl("tabs")] = tabsArray;

    QJsonDocument jsonDoc(rootObject);

    QSaveFile file;
    file.setFileName(fileName);
    if (!file.open(QIODevice::WriteOnly)) {
        qDebug() << "NotesManager::save: failed to open file for writing:" << file.errorString();
        return;
    }

    file.write(jsonDoc.toJson(QJsonDocument::Indented));

    if (!file.commit()) {
        qDebug() << "NotesManager::save: error saving notes contents: " << file.errorString();
        return;
    }

    for (auto it = mTabs.begin(); it != mTabs.end(); ++it) {
        it.value().isDirty = false;
    }

    mNeedToSave = false;
}

void NotesManager::restore()
{
    if (!mpHost) {
        return;
    }

    const QString fileName = mudlet::getMudletPath(enums::profileDataItemPath, mpHost->getName(), jsonEncodedNotesFileName);

    if (QFile::exists(fileName)) {
        QFile file(fileName);
        if (!file.open(QIODevice::ReadOnly)) {
            qDebug() << "NotesManager::restore: failed to open file for reading:" << file.errorString();
            migrateFromOldFormat();
            return;
        }

        const QByteArray jsonData = file.readAll();
        file.close();

        QJsonParseError parseError;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData, &parseError);

        if (parseError.error != QJsonParseError::NoError) {
            qDebug() << "NotesManager::restore: JSON parse error:" << parseError.errorString();
            migrateFromOldFormat();
            return;
        }

        QJsonObject rootObject = jsonDoc.object();
        if (!rootObject.contains(qsl("tabs")) || !rootObject[qsl("tabs")].isArray()) {
            qDebug() << "NotesManager::restore: invalid JSON structure";
            migrateFromOldFormat();
            return;
        }

        QJsonArray tabsArray = rootObject[qsl("tabs")].toArray();
        mTabs.clear();

        for (const QJsonValue& tabValue : tabsArray) {
            QJsonObject tabObject = tabValue.toObject();
            if (!tabObject.contains(qsl("id")) || !tabObject.contains(qsl("name"))) {
                continue;
            }

            NoteTab tab;
            const QString tabId = tabObject[qsl("id")].toString();
            tab.name = tabObject[qsl("name")].toString();
            tab.content = tabObject[qsl("content")].toString();
            const QString lastModifiedStr = tabObject[qsl("lastModified")].toString();
            tab.lastModified = QDateTime::fromString(lastModifiedStr, Qt::ISODate);
            tab.isDirty = false;

            mTabs.insert(tabId, tab);
        }

        if (mTabs.isEmpty()) {
            initializeDefaultTab();
        }
    } else {
        migrateFromOldFormat();
    }
}

void NotesManager::saveSettings()
{
}

void NotesManager::restoreSettings()
{
}

void NotesManager::initializeDefaultTab()
{
    if (!mTabs.isEmpty()) {
        return;
    }

    const QString tabId = generateTabId();
    NoteTab tab;
    tab.name = tr("General Notes");
    tab.content = QString();
    tab.lastModified = QDateTime::currentDateTimeUtc();
    tab.isDirty = false;

    mTabs.insert(tabId, tab);
    mNeedToSave = true;
}

QString NotesManager::generateTabId()
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

void NotesManager::migrateFromOldFormat()
{
    if (!mpHost) {
        return;
    }

    QString oldFileName;
    bool useUtf8Encoding = false;

    QString utf8FileName = mudlet::getMudletPath(enums::profileDataItemPath, mpHost->getName(), utf8EncodedNotesFileName);
    if (QFile::exists(utf8FileName)) {
        oldFileName = utf8FileName;
        useUtf8Encoding = true;
    } else {
        QString localFileName = mudlet::getMudletPath(enums::profileDataItemPath, mpHost->getName(), local8BitEncodedNotesFileName);
        if (QFile::exists(localFileName)) {
            oldFileName = localFileName;
            useUtf8Encoding = false;
        }
    }

    if (oldFileName.isEmpty()) {
        initializeDefaultTab();
        return;
    }

    QFile file(oldFileName);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "NotesManager::migrateFromOldFormat: failed to open old notes file for reading:" << file.errorString();
        initializeDefaultTab();
        return;
    }

    QTextStream fileStream;
    fileStream.setDevice(&file);
    if (!useUtf8Encoding) {
        fileStream.setEncoding(QStringEncoder::Encoding::System);
    }

    const QString content = fileStream.readAll();
    file.close();

    const QString tabId = generateTabId();
    NoteTab tab;
    tab.name = tr("General Notes");
    tab.content = content;
    tab.lastModified = QDateTime::currentDateTimeUtc();
    tab.isDirty = true;

    mTabs.insert(tabId, tab);
    mNeedToSave = true;

    save();
}
