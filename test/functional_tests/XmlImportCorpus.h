#ifndef MUDLET_XMLIMPORTCORPUS_H
#define MUDLET_XMLIMPORTCORPUS_H

/***************************************************************************
 *   Copyright (C) 2026 by Vadim Peretokin - vadim.peretokin@mudlet.org    *
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

#include <QCryptographicHash>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QList>
#include <QSet>
#include <QTemporaryDir>

// Every package, module, profile and map XML in the source tree, archives unpacked
namespace XmlImportCorpus {

struct Entry
{
    QString name;
    QByteArray bytes;
};

inline QStringList filesUnder(const QString& directory, const QStringList& patterns)
{
    QStringList files;
    QDirIterator it(directory, patterns, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        files << it.next();
    }
    files.sort();
    return files;
}

inline QList<Entry> load(const QString& sourceDir, const QTemporaryDir& unpackDir)
{
    const QString fixtures = qsl("%1/mudlet-lua/tests/fixtures").arg(sourceDir);
    QStringList xmlFiles = filesUnder(qsl("%1/packages").arg(sourceDir), {qsl("*.xml")});
    xmlFiles << qsl("%1/mudlet-mapper.xml").arg(sourceDir);
    xmlFiles << filesUnder(qsl("%1/packages").arg(fixtures), {qsl("*.xml")});
    xmlFiles << filesUnder(qsl("%1/maps").arg(fixtures), {qsl("*.xml")});

    QStringList archives = filesUnder(qsl("%1/packages").arg(sourceDir), {qsl("*.mpackage")});
    archives << filesUnder(qsl("%1/packages").arg(fixtures), {qsl("*.mpackage")});
    archives << filesUnder(qsl("%1/maps").arg(fixtures), {qsl("*.zip")});
    archives << qsl("%1/../test/GUIConsoleTests.mpackage").arg(sourceDir);
    // More to compare, such as a checkout of the Mudlet package repository
    if (const QString extra = qEnvironmentVariable("MUDLET_XML_IMPORT_CORPUS"); !extra.isEmpty()) {
        xmlFiles << filesUnder(extra, {qsl("*.xml")});
        archives << filesUnder(extra, {qsl("*.mpackage"), qsl("*.zip")});
    }
    int unpacked = 0;
    for (const QString& archive : std::as_const(archives)) {
        const QString destination = qsl("%1/%2/").arg(unpackDir.path()).arg(unpacked++);
        if (QDir().mkpath(destination) && utils::unzip(archive, destination, QDir(unpackDir.path()))) {
            xmlFiles << filesUnder(destination, {qsl("*.xml")});
        }
    }

    QList<Entry> corpus;
    QSet<QByteArray> seen;
    for (const QString& path : std::as_const(xmlFiles)) {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly)) {
            continue;
        }
        QByteArray bytes = file.readAll();
        if (seen.contains(QCryptographicHash::hash(bytes, QCryptographicHash::Sha1))) {
            continue;
        }
        seen.insert(QCryptographicHash::hash(bytes, QCryptographicHash::Sha1));
        QString name = path;
        name.remove(unpackDir.path()).remove(sourceDir);
        corpus.append({name, bytes});
    }
    return corpus;
}

} // namespace XmlImportCorpus

#endif // MUDLET_XMLIMPORTCORPUS_H
