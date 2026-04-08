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

// QObject must be included before Q_OS_MACOS checks below
#include <QObject>

// Guard for builds without the updater (INCLUDE_UPDATER is defined by CMake when USE_UPDATER is ON):
#if defined(INCLUDE_UPDATER)
namespace dblsqd {
class Feed;
class Release;
class UpdateDialog;
}
#if defined(Q_OS_MACOS)
#include "sparkleupdater.h"
#endif
#endif

class QAbstractButton;
class QPushButton;
class QSettings;
class QTimer;

class Updater : public QObject
{
    Q_OBJECT

public:
    Q_DISABLE_COPY(Updater)
    explicit Updater(QObject* parent = nullptr, QSettings* settings = nullptr, bool testVersion = false);
    virtual ~Updater();
    void checkUpdatesOnStart();
    void manuallyCheckUpdates();
    void showChangelog() const;
    void showFullChangelog() const;
    void setAutomaticUpdates(bool state);
    bool updateAutomatically() const;
    bool shouldShowChangelog();

private:
    dblsqd::Feed* mFeed;
    dblsqd::UpdateDialog* mUpdateDialog{nullptr};
#if !defined(Q_OS_MACOS)
    QPushButton* mpInstallOrRestart;
#endif
    bool mUpdateInstalled;
    bool mManualCheckInProgress{false};
    QSettings* mSettings;
    std::unique_ptr<QTimer> mPeriodicCheck;

#if defined(Q_OS_LINUX)
    void setupOnLinux();
    void untarOnLinux(const QString& fileName);
#elif defined(Q_OS_WINDOWS)
    void setupOnWindows();
    void prepareSetupOnWindows(const QString& fileName);
#elif defined(Q_OS_MACOS)
    void setupOnMacOS();
#endif

#if !defined(Q_OS_MACOS)
    void setupPlatformUpdater();
#endif
    void recordUpdateTime() const;
    void recordUpdatedVersion() const;
    QString getPreviousVersion() const;
    bool downloadReleaseIfValid(const dblsqd::Release& release);
    void finishSetup();
    void showDialogManually() const;

#if defined(Q_OS_LINUX)
    QString mUnzippedBinaryName;
#elif defined(Q_OS_WINDOWS)
    QString mDownloadedInstallerPath;
#elif defined(Q_OS_MACOS)
    SparkleUpdater* msparkleUpdater;
#endif


signals:
    void signal_updateInstalled();
    void signal_updateAvailable(const int);
    void signal_automaticUpdatesChanged(const bool);
    void signal_updateCheckFailed(const QString& error);

public slots:
    void slot_installOrRestartClicked(QAbstractButton* button, const QString& filePath);
#if defined(Q_OS_LINUX)
    void slot_updateLinuxBinary();
#endif
};

#endif // UPDATER_H
