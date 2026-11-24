#ifndef MUDLET_REPLACEDBINARYWATCHER_H
#define MUDLET_REPLACEDBINARYWATCHER_H

/***************************************************************************
 *   Copyright (C) 2026 by Mudlet Developers                               *
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

#include <QFileSystemWatcher>
#include <QObject>
#include <QString>
#include <QTimer>

#include <optional>

#include <sys/types.h>

class QProcess;

// Notices when a package manager replaces the binary this process runs from, for builds
// without the updater: dpkg and rpm rename a new file over the old one, so the path's
// inode stops matching the running one, and the new binary is asked for its version.
class ReplacedBinaryWatcher : public QObject
{
    Q_OBJECT

public:
    // runningBinary is stat()ed for the inode this process runs from; Mudlet passes
    // /proc/self/exe, which still reaches that inode after the file is replaced
    ReplacedBinaryWatcher(const QString& installedPath, const QString& runningBinary, const QString& runningVersion, QObject* parent = nullptr);

    static QString versionFromOutput(const QString& output);

signals:
    void signal_newVersionInstalled(const QString& version);

private:
    struct FileId
    {
        dev_t device = 0;
        ino_t inode = 0;
        bool operator==(const FileId&) const = default;
    };
    static std::optional<FileId> fileIdOf(const QString& path);
    void checkInstalled();
    void versionReplied();
    void abandonQuery();

    QString mInstalledPath;
    QString mRunningVersion;
    QString mAnnouncedVersion;
    FileId mRunningId;
    // The installed file whose version is being, or has been, read, so it is asked only once
    FileId mQueriedId;
    FileId mFailingId;
    int mFailures = 0;
    QFileSystemWatcher mWatcher;
    QTimer mSettleTimer;
    QTimer mRetryTimer;
    QProcess* mpVersionQuery = nullptr;
};

#endif // MUDLET_REPLACEDBINARYWATCHER_H
