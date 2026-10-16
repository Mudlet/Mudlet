#ifndef MUDLET_MUDLETWEBEXPORT_H
#define MUDLET_MUDLETWEBEXPORT_H

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

#include <QDateTime>
#include <QObject>
#include <QPointer>
#include <QStringList>

class Host;

// Packs a profile into the .zip that Mudlet Web's "Import .zip" reads: the
// profile folder as desktop keeps it, with a fresh save and map, plus the
// modules - which live elsewhere on disk - copied in under "<module>/".
class MudletWebExport : public QObject
{
    Q_OBJECT

public:
    Q_DISABLE_COPY(MudletWebExport)
    MudletWebExport(Host*, const QString& archivePathFileName, QObject* parent = nullptr);

    // Saves the profile and writes the archive once that save is on disk.
    // finished() is emitted exactly once, always from the event loop.
    void start();

    static QString suggestedFileName(const QString& profileName);
    static const QString scmMudletWebUrl;

signals:
    void finished(bool ok, const QString& error, const QStringList& warnings);

private:
    void saveThenWrite();
    void writeArchive(const QString& profileXmlPathFileName);
    void finish(bool ok, const QString& error);

    QPointer<Host> mpHost;
    QString mArchivePathFileName;
    QStringList mWarnings;
    QMetaObject::Connection mHostGone;
    QDateTime mSaveStarted;
    bool mFinished = false;
};

#endif // MUDLET_MUDLETWEBEXPORT_H
