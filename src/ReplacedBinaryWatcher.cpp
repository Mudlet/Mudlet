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

#include "ReplacedBinaryWatcher.h"

#include "utils.h"

#include <QFile>
#include <QFileInfo>
#include <QProcess>

#include <utility>

#include <sys/stat.h>

using namespace std::chrono_literals;

// Asked again 5s, 10s, 20s, 40s and 80s after failing, then given up on until it is replaced again
static constexpr int csMaxFailures = 6;
static constexpr std::chrono::seconds csFirstRetryDelay = 5s;

ReplacedBinaryWatcher::ReplacedBinaryWatcher(const QString& installedPath, const QString& runningBinary, const QString& runningVersion, QObject* parent)
: QObject(parent)
, mInstalledPath(installedPath)
, mRunningVersion(runningVersion)
{
    const auto runningId = fileIdOf(runningBinary);
    if (!runningId) {
        return;
    }
    mRunningId = *runningId;

    // One install changes the directory several times: its temporary file appears, then is renamed into place
    mSettleTimer.setSingleShot(true);
    mSettleTimer.setInterval(1s);
    connect(&mSettleTimer, &QTimer::timeout, this, &ReplacedBinaryWatcher::checkInstalled);
    mRetryTimer.setSingleShot(true);
    connect(&mRetryTimer, &QTimer::timeout, this, &ReplacedBinaryWatcher::checkInstalled);

    // The directory, not the file: a watch on the file is lost when another is renamed over it
    mWatcher.addPath(QFileInfo(installedPath).absolutePath());
    connect(&mWatcher, &QFileSystemWatcher::directoryChanged, &mSettleTimer, qOverload<>(&QTimer::start));
}

std::optional<ReplacedBinaryWatcher::FileId> ReplacedBinaryWatcher::fileIdOf(const QString& path)
{
    struct stat info{};
    if (::stat(QFile::encodeName(path).constData(), &info) != 0) {
        return std::nullopt;
    }
    return FileId{info.st_dev, info.st_ino};
}

// The first line of --version: "mudlet 5.1.0" from a release, "mudlet 5.1.0 -ptb-abc1234" from a
// public test build, and "mudlet 5.1.0-dev-abc1234 (with debug symbols, ...)" from a debug build
QString ReplacedBinaryWatcher::versionFromOutput(const QString& output)
{
    const QStringList words = output.section(QLatin1Char('\n'), 0, 0).split(QLatin1Char(' '), Qt::SkipEmptyParts);
    if (words.size() < 2) {
        return {};
    }
    if (words.size() > 2 && words.at(2).startsWith(QLatin1Char('-'))) {
        return words.at(1) + words.at(2);
    }
    return words.at(1);
}

void ReplacedBinaryWatcher::checkInstalled()
{
    if (mpVersionQuery) {
        return;
    }
    const auto installedId = fileIdOf(mInstalledPath);
    if (!installedId || *installedId == mRunningId || *installedId == mQueriedId) {
        return;
    }
    mQueriedId = *installedId;

    mpVersionQuery = new QProcess(this);
    connect(mpVersionQuery, &QProcess::finished, this, &ReplacedBinaryWatcher::versionReplied);
    connect(mpVersionQuery, &QProcess::errorOccurred, this, [this](const QProcess::ProcessError error) {
        // The only error after which finished() never comes
        if (error == QProcess::FailedToStart) {
            abandonQuery();
        }
    });
    QTimer::singleShot(10s, mpVersionQuery, &QProcess::kill);
    mpVersionQuery->start(mInstalledPath, {qsl("--version")});
}

void ReplacedBinaryWatcher::versionReplied()
{
    if (mpVersionQuery->exitStatus() != QProcess::NormalExit || mpVersionQuery->exitCode() != 0) {
        abandonQuery();
        return;
    }
    const QString version = versionFromOutput(QString::fromLocal8Bit(mpVersionQuery->readAllStandardOutput()));
    std::exchange(mpVersionQuery, nullptr)->deleteLater();
    if (!version.isEmpty() && version != mRunningVersion && version != mAnnouncedVersion) {
        mAnnouncedVersion = version;
        emit signal_newVersionInstalled(version);
    }
    // In case it was replaced again while this one was being asked
    checkInstalled();
}

void ReplacedBinaryWatcher::abandonQuery()
{
    std::exchange(mpVersionQuery, nullptr)->deleteLater();
    if (mQueriedId != mFailingId) {
        mFailingId = mQueriedId;
        mFailures = 0;
    }
    mQueriedId = {};
    // An install still unpacking may not run yet, and what it waits for can land outside the watched directory
    if (++mFailures < csMaxFailures) {
        mRetryTimer.start(csFirstRetryDelay * (1 << (mFailures - 1)));
    }
}
