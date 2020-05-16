/***************************************************************************
 *   Copyright (C) 2017-2020 by Vadim Peretokin - vperetokin@gmail.com     *
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

#ifndef UPDATER_H
#define UPDATER_H

// FreeBSD does not support the updater and these missing files upset
// clang-tidy / Clazy when they are run in an environment without them:
#if defined (INCLUDE_UPDATER)
#include "dblsqd/feed.h"
#include "dblsqd/update_dialog.h"
#endif

#ifdef Q_OS_MACOS
#include "../3rdparty/sparkle-glue/AutoUpdater.h"
#endif

#include "pre_guard.h"
#include <QObject>
#include "post_guard.h"

class Updater : public QObject
{
    Q_OBJECT

public:
    Q_DISABLE_COPY(Updater)
    explicit Updater(QObject* parent = nullptr, QSettings* settings = nullptr);
    virtual ~Updater();
    void checkUpdatesOnStart();
    void manuallyCheckUpdates();
    void showChangelog() const;
    void setAutomaticUpdates(bool state);
    bool updateAutomatically() const;
    bool shouldShowChangelog();

private:
    dblsqd::Feed* feed;
    dblsqd::UpdateDialog* updateDialog;
    QPushButton* mpInstallOrRestart;
    bool mUpdateInstalled;
    QSettings* settings;
    std::unique_ptr<QTimer> mDailyCheck;

#if defined(Q_OS_LINUX)
    void setupOnLinux();
    void untarOnLinux(const QString& fileName);
#elif defined(Q_OS_WIN32)
    void setupOnWindows();
    void prepareSetupOnWindows(const QString& fileName);
#elif defined(Q_OS_MACOS)
    void setupOnMacOS();
#endif

    void recordUpdateTime() const;
    void recordUpdatedVersion() const;
    QString getPreviousVersion() const;
    void finishSetup();
    void showDialogManually() const;

#if defined(Q_OS_LINUX)
    QString unzippedBinaryName;
#elif defined(Q_OS_MACOS)
    AutoUpdater* msparkleUpdater;
#endif


signals:
    void signal_updateInstalled();
    // Argument is a count of updates available
    void signal_updateAvailable(const int);
    void signal_automaticUpdatesChanged(const bool);

public slots:
    void installOrRestartClicked(QAbstractButton* button, const QString& filePath);
#if defined(Q_OS_LINUX)
    // might want to make these private
    void updateBinaryOnLinux();
#endif
};

#endif // UPDATER_H
