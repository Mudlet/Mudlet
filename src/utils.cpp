/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2013-2026 by Stephen Lyons - slysven@virginmedia.com    *
 *   Copyright (C) 2011-2026 by Vadim Peretokin - vperetokin@hey.com       *
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
#include "utils.h"

#include "TGameDetails.h"
#include "enums.h"
#include "mudlet.h"

#include <QDataStream>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QSaveFile>


QString utils::readProfileData(const QString& profile, const QString& item)
{
    QFile file(mudlet::getMudletPath(enums::profileDataItemPath, profile, item));
    if (!file.exists()) {
        return QString();
    }

    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "mudlet: failed to open profile data file for reading:" << file.fileName() << file.errorString();
        return QString();
    }

    QDataStream ifs(&file);
    ifs.setVersion(QDataStream::Qt_5_12);
    QString ret;

    ifs >> ret;
    file.close();
    return ret;
}

QPair<bool, QString> utils::writeProfileData(const QString& profile, const QString& item, const QString& what)
{
    const QDir profileDir;
    const QString profileHomePath = mudlet::getMudletPath(enums::profileHomePath, profile);
    if (!QDir(profileHomePath).exists() && !profileDir.mkpath(profileHomePath)) {
        qDebug().noquote().nospace() << "utils::writeProfileData(...) ERROR - could not create profile directory: \"" << profileHomePath << "\"";
        return qMakePair(false, qsl("Could not create profile directory: %1").arg(profileHomePath));
    }

    QSaveFile file(mudlet::getMudletPath(enums::profileDataItemPath, profile, item));
    if (file.open(QIODevice::WriteOnly | QIODevice::Unbuffered)) {
        QDataStream ofs(&file);
        ofs.setVersion(QDataStream::Qt_5_12);
        ofs << what;
        if (!file.commit()) {
            qDebug().noquote().nospace() << "utils::writeProfileData(...) ERROR - writing profile: \"" << profile << "\", item: \"" << item << "\", reason: \"" << file.errorString() << "\".";
        }
    }

    if (file.error() == QFile::NoError) {
        return qMakePair(true, QString());
    }

    return qMakePair(false, file.errorString());
}

QString utils::getCanonicalProfileName(const QString& profileName)
{
    if (profileName.isEmpty()) {
        return QString();
    }

    const QStringList profiles = QDir(mudlet::getMudletPath(enums::profilesPath)).entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
    for (const auto& profile : profiles) {
        if (profile.compare(profileName, Qt::CaseInsensitive) == 0) {
            return profile;
        }
    }

    const auto it = TGameDetails::findGame(profileName, Qt::CaseInsensitive);
    if (it != TGameDetails::scmDefaultGames.constEnd()) {
        return it->name;
    }

    return QString();
}
