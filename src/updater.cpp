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

#include "updater.h"
#include "mudlet.h"

#if defined(Q_OS_MACOS)
#include "../3rdparty/sparkle-glue/CocoaInitializer.h"
#include "../3rdparty/sparkle-glue/SparkleAutoUpdater.h"
#endif

#include "pre_guard.h"
#include <QPushButton>
#include <QtConcurrent>
#include <chrono>
#include "post_guard.h"

using namespace std::chrono_literals;

// update flows:
// linux: new AppImage is downloaded, unzipped, and put in place of the old one
//   user then only restarts mudlet to get the new version
// windows: new squirrel installer is downloaded and saved
//   user then restarts, mudlet sees that there's a new installer available: launches it
//   and promptly quits. Installer updates Mudlet and launches Mudlet when its done
// mac: handled completely outside of Mudlet by Sparkle

Updater::Updater(QObject* parent, QSettings* settings) : QObject(parent)
, updateDialog(nullptr)
, mpInstallOrRestart(new QPushButton(tr("Update")))
, mUpdateInstalled(false)
{
    Q_ASSERT_X(settings, "updater", "QSettings object is required for the updater to work");
    this->settings = settings;

    feed = new dblsqd::Feed(QStringLiteral("https://feeds.dblsqd.com/MKMMR7HNSP65PquQQbiDIw"),
                            mudlet::scmIsPublicTestVersion ? QStringLiteral("public-test-build") : QStringLiteral("release"));

    if (!mDailyCheck) {
        mDailyCheck = std::make_unique<QTimer>();
    }
}
Updater::~Updater()
{
    delete (feed);
}

// start the update process and figure out what needs to be done.
// If it's silent updates, do that right away, otherwise
// setup manual updates to do our custom actions
void Updater::checkUpdatesOnStart()
{
#if defined(Q_OS_MACOS)
    setupOnMacOS();
#elif defined(Q_OS_LINUX)
    setupOnLinux();
#elif defined(Q_OS_WIN32)
    setupOnWindows();
#endif

    mDailyCheck->setInterval(12h);
    QObject::connect(mDailyCheck.get(), &QTimer::timeout, this, [this] {
          auto updates = feed->getUpdates(dblsqd::Release::getCurrentRelease());
          qWarning() << "Daily check for updates:" << updates.size() << "update(s) available";
          if (updates.isEmpty()) {
              return;
          } else if (!updateAutomatically()) {
              emit signal_updateAvailable(updates.size());
          } else {
              feed->downloadRelease(updates.first());
          }
    });
    mDailyCheck->start();
}

void Updater::setAutomaticUpdates(const bool state)
{
#if defined(Q_OS_MACOS)
    msparkleUpdater->setAutomaticallyDownloadsUpdates(state);
#else
    dblsqd::UpdateDialog::enableAutoDownload(state, settings);
#endif
    // The sense of this control is inverted on the dlgProfilePreferences - so
    // must be inverted here:
    emit signal_automaticUpdatesChanged(!state);
}

bool Updater::updateAutomatically() const
{
#if defined(Q_OS_MACOS)
    return msparkleUpdater->automaticallyDownloadsUpdates();
#else
    return dblsqd::UpdateDialog::autoDownloadEnabled(true, settings);
#endif
}

void Updater::manuallyCheckUpdates()
{
#if defined(Q_OS_MACOS)
    msparkleUpdater->checkForUpdates();
#else
    feed->load();
    QObject::connect(feed, &dblsqd::Feed::ready, this, &Updater::showDialogManually);
#endif
}

void Updater::showDialogManually() const
{
    updateDialog->show();
    QObject::disconnect(feed, &dblsqd::Feed::ready, this, &Updater::showDialogManually);
}

void Updater::showChangelog() const
{
    auto changelogDialog = new dblsqd::UpdateDialog(feed, dblsqd::UpdateDialog::ManualChangelog);
    changelogDialog->setPreviousVersion(getPreviousVersion());
    changelogDialog->show();
}

void Updater::finishSetup()
{
#if defined(Q_OS_LINUX)
    qWarning() << "Successfully updated Mudlet to" << feed->getUpdates(dblsqd::Release::getCurrentRelease()).constFirst().getVersion();
#elif defined(Q_OS_WIN32)
    qWarning() << "Mudlet prepped to update to" << feed->getUpdates(dblsqd::Release::getCurrentRelease()).first().getVersion() << "on restart";
#endif
    recordUpdateTime();
    recordUpdatedVersion();
    mUpdateInstalled = true;
    emit signal_updateInstalled();
}

#if defined(Q_OS_MACOS)
void Updater::setupOnMacOS()
{
    CocoaInitializer initializer;
    msparkleUpdater = new SparkleAutoUpdater(QStringLiteral("https://feeds.dblsqd.com/MKMMR7HNSP65PquQQbiDIw/release/mac/x86_64/appcast"));
    // don't need to explicitly check for updates - sparkle will do so on its own
}
#endif // Q_OS_MACOS

#if defined(Q_OS_WIN32)
void Updater::setupOnWindows()
{
    // Setup to automatically download the new release when an update is available
    QObject::connect(feed, &dblsqd::Feed::ready, [=]() {
        if (mudlet::scmIsDevelopmentVersion) {
            return;
        }

        auto updates = feed->getUpdates(dblsqd::Release::getCurrentRelease());
        qWarning() << "Checked for updates:" << updates.size() << "update(s) available";
        if (updates.isEmpty()) {
            return;
        } else if (!updateAutomatically()) {
            emit signal_updateAvailable(updates.size());
        } else {
            feed->downloadRelease(updates.first());
        }
    });

    // Setup to run setup.exe to replace the old installation
    QObject::connect(feed, &dblsqd::Feed::downloadFinished, [=]() {
        // if automatic updates are enabled, and this isn't a manual check, perform the automatic update
        if (!(updateAutomatically() && updateDialog->isHidden())) {
            return;
        }

        QFuture<void> future = QtConcurrent::run(this, &Updater::prepareSetupOnWindows, feed->getDownloadFile()->fileName());

        // replace current binary with the unzipped one
        auto watcher = new QFutureWatcher<void>;
        connect(watcher, &QFutureWatcher<void>::finished, this, &Updater::finishSetup);
        watcher->setFuture(future);
    });

    // finally, create the dblsqd objects. Constructing the UpdateDialog triggers the update check
    updateDialog = new dblsqd::UpdateDialog(feed, updateAutomatically() ? dblsqd::UpdateDialog::OnLastWindowClosed : dblsqd::UpdateDialog::Manual, nullptr, settings);
    mpInstallOrRestart->setText(tr("Update"));
    updateDialog->addInstallButton(mpInstallOrRestart);
    connect(updateDialog, &dblsqd::UpdateDialog::installButtonClicked, this, &Updater::installOrRestartClicked);
}

// moved the new updater to the same directory as mudlet.exe so it is run on the next
// launch. Don't run it ourselves right now since it insists on launching Mudlet when it's done
void Updater::prepareSetupOnWindows(const QString& downloadedSetupName)
{
    QDir dir;
    auto newPath = QString(QCoreApplication::applicationDirPath() + QStringLiteral("/new-mudlet-setup.exe"));
    QFileInfo newPathFileInfo(newPath);
    if (newPathFileInfo.exists() && !dir.remove(newPathFileInfo.absoluteFilePath())) {
        qDebug() << "Couldn't delete the old installer";
    }

    // dir.rename actually moves a file
    if (!dir.rename(downloadedSetupName, newPath)) {
        qWarning() << "Moving new installer into " << newPath << "failed";
        return;
    }
}
#endif // Q_OS_WIN

#if defined(Q_OS_LINUX)
void Updater::setupOnLinux()
{
    // Setup to automatically download the new release when an update is
    // available or wave a flag when it is to be done manually
    // Setup to automatically download the new release when an update is available
    QObject::connect(feed, &dblsqd::Feed::ready, this, [=]() {
        // don't update development builds to prevent auto-update from overwriting your
        // compiled binary while in development
        if (mudlet::scmIsDevelopmentVersion) {
            return;
        }

        auto updates = feed->getUpdates(dblsqd::Release::getCurrentRelease());
        qWarning() << "Checked for updates:" << updates.size() << "update(s) available";
        if (updates.isEmpty()) {
            return;
        } else if (!updateAutomatically()) {
            emit signal_updateAvailable(updates.size());
            return;
        } else {
            feed->downloadRelease(updates.first());
        }
    });

    // Setup to unzip and replace old binary when the download is done
    QObject::connect(feed, &dblsqd::Feed::downloadFinished, this, [=]() {
        // if automatic updates are enabled, and this isn't a manual check, perform the automatic update
        if (!(updateAutomatically() && updateDialog->isHidden())) {
            return;
        }

        QFuture<void> future = QtConcurrent::run(this, &Updater::untarOnLinux, feed->getDownloadFile()->fileName());

        // replace current binary with the unzipped one
        auto watcher = new QFutureWatcher<void>;
        connect(watcher, &QFutureWatcher<void>::finished, this, &Updater::updateBinaryOnLinux);
        watcher->setFuture(future);
    });

    // finally, create the dblsqd objects. Constructing the UpdateDialog triggers the update check
    updateDialog = new dblsqd::UpdateDialog(feed, updateAutomatically() ? dblsqd::UpdateDialog::OnLastWindowClosed : dblsqd::UpdateDialog::Manual, nullptr, settings);
    mpInstallOrRestart->setText(tr("Update"));
    updateDialog->addInstallButton(mpInstallOrRestart);
    connect(updateDialog, &dblsqd::UpdateDialog::installButtonClicked, this, &Updater::installOrRestartClicked);
}

void Updater::untarOnLinux(const QString& fileName)
{
    Q_ASSERT_X(QThread::currentThread() != QCoreApplication::instance()->thread(), "untarOnLinux", "method should not be called in the main GUI thread to avoid a degradation in UX");

    QProcess tar;
    tar.setProcessChannelMode(QProcess::MergedChannels);
    // we can assume tar to be present on a Linux system. If it's not, it'd be rather broken.
    // tar output folder has to end with a slash
    tar.start(QStringLiteral("tar"), QStringList() << QStringLiteral("-xvf") << fileName << QStringLiteral("-C") << QStandardPaths::writableLocation(QStandardPaths::TempLocation) + QStringLiteral("/"));
    if (!tar.waitForFinished()) {
        qWarning() << "Untarring" << fileName << "failed:" << tar.errorString();
    } else {
        unzippedBinaryName = tar.readAll().trimmed();
    }
}

void Updater::updateBinaryOnLinux()
{
    QFileInfo unzippedBinary(QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/" + unzippedBinaryName);
    auto systemEnvironment = QProcessEnvironment::systemEnvironment();
    auto appimageLocation = systemEnvironment.contains(QStringLiteral("APPIMAGE")) ?
                systemEnvironment.value(QStringLiteral("APPIMAGE"), QString()) :
                QCoreApplication::applicationFilePath();

    const QString& installedBinaryPath(appimageLocation);

    auto executablePermissions = unzippedBinary.permissions();
    executablePermissions |= QFileDevice::ExeOwner | QFileDevice::ExeUser;

    QDir dir;
    // dir.rename actually moves a file
    if (!(dir.remove(installedBinaryPath) && dir.rename(unzippedBinary.filePath(), installedBinaryPath))) {
        qWarning() << "updating" << installedBinaryPath << "with new version from" << unzippedBinary.filePath() << "failed";
        return;
    }

    QFile updatedBinary(appimageLocation);
    if (!updatedBinary.setPermissions(executablePermissions)) {
        qWarning() << "couldn't set executable permissions on updated Mudlet binary at" << installedBinaryPath;
        return;
    }

    finishSetup();
}
#endif // Q_OS_LINUX

void Updater::installOrRestartClicked(QAbstractButton* button, const QString& filePath)
{
    Q_UNUSED(button)

    // moc, when used with cmake on macOS bugs out if the entire function declaration and definition is entirely
    // commented out so we leave a stub in
#if !defined(Q_OS_MACOS)

    // if the update is already installed, then the button says 'Restart' - do so
    if (mUpdateInstalled) {
        // timer is necessary as calling close right way doesn't seem to do the trick
        QTimer::singleShot(0, this, [=]() {
            updateDialog->close();
            updateDialog->done(0);
        });

        // if the updater is launched manually instead of when Mudlet is quit,
        // close Mudlet ourselves
        if (mudlet::self()) {
            mudlet::self()->forceClose();
        }
        QProcess::startDetached(qApp->arguments()[0], qApp->arguments());
        return;
    }

// otherwise the button says 'Install', so install the update
#if defined(Q_OS_LINUX)
    QFuture<void> future = QtConcurrent::run(this, &Updater::untarOnLinux, filePath);
#elif defined(Q_OS_WIN32)
    QFuture<void> future = QtConcurrent::run(this, &Updater::prepareSetupOnWindows, filePath);
#endif

    // replace current binary with the unzipped one
    auto watcher = new QFutureWatcher<void>;
    connect(watcher, &QFutureWatcher<void>::finished, this, [=]() {
#if defined(Q_OS_LINUX)
        updateBinaryOnLinux();
#elif defined(Q_OS_WIN32)
        finishSetup();
#endif
        mpInstallOrRestart->setText(tr("Restart to apply update"));
        mpInstallOrRestart->setEnabled(true);
    });
    watcher->setFuture(future);
#endif // !Q_OS_MACOS
}

// records a unix epoch on disk indicating that an update has happened.
// Mudlet will use that on the next launch to decide whenever it should show
// the window with the new features. The idea is that if you manually update (thus see the
// changelog already) and restart, you shouldn't see it again, and if you automatically
// updated, then you do want to see the changelog.
void Updater::recordUpdateTime() const
{
    QFile file(mudlet::getMudletPath(mudlet::mainDataItemPath, QStringLiteral("mudlet_updated_at")));
    bool opened = file.open(QIODevice::WriteOnly);
    if (!opened) {
        qWarning() << "Couldn't open update timestamp file for writing.";
        return;
    }

    QDataStream ifs(&file);
    if (mudlet::scmRunTimeQtVersion >= QVersionNumber(5, 13, 0)) {
        ifs.setVersion(mudlet::scmQDataStreamFormat_5_12);
    }
    ifs << QDateTime::currentDateTime().toMSecsSinceEpoch();
    file.close();
}

// records the previous version of Mudlet that we updated from, so we can show
// the changelog on next startup for the latest version only
void Updater::recordUpdatedVersion() const
{
    QFile file(mudlet::getMudletPath(mudlet::mainDataItemPath, QStringLiteral("mudlet_updated_from")));
    bool opened = file.open(QIODevice::WriteOnly);
    if (!opened) {
        qWarning() << "Couldn't open update version file for writing.";
        return;
    }

    QDataStream ifs(&file);
    if (mudlet::scmRunTimeQtVersion >= QVersionNumber(5, 13, 0)) {
        ifs.setVersion(mudlet::scmQDataStreamFormat_5_12);
    }
    ifs << APP_VERSION;
    file.close();
}

// returns true if Mudlet was updated automatically and a changelog should be shown
// now that the user is on the new version. If the user updated manually, then there
// is no need as they would have seen the changelog while updating
bool Updater::shouldShowChangelog()
{
// Don't show changelog for automatic updates on Sparkle - Sparkle doesn't support it
#if defined(Q_OS_MACOS)
    return false;
#endif

    if (mudlet::scmIsDevelopmentVersion || !updateAutomatically()) {
        return false;
    }

    QFile file(mudlet::self()->getMudletPath(mudlet::mainDataItemPath, QStringLiteral("mudlet_updated_at")));
    bool opened = file.open(QIODevice::ReadOnly);
    qint64 updateTimestamp;
    if (!opened) {
        file.remove();
        return false;
    }
    QDataStream ifs(&file);
    if (mudlet::scmRunTimeQtVersion >= QVersionNumber(5, 13, 0)) {
        ifs.setVersion(mudlet::scmQDataStreamFormat_5_12);
    }
    ifs >> updateTimestamp;
    file.close();

    auto currentDateTime = QDateTime::currentDateTime().toMSecsSinceEpoch();
    auto minsSinceUpdate = (currentDateTime - updateTimestamp) / 1000 / 60;

    // delete the file on check as well since if we updated and restarted right away
    // we won't need to show the changelog - as well as on a launch 5mins after.
    file.remove();

    return minsSinceUpdate >= 5;
}

// return the previous version of Mudlet that we updated from
// return a null QString on failure
QString Updater::getPreviousVersion() const
{
    QFile file(mudlet::self()->getMudletPath(mudlet::mainDataItemPath, QStringLiteral("mudlet_updated_from")));
    bool opened = file.open(QIODevice::ReadOnly);
    QString previousVersion;
    if (!opened) {
        file.remove();
        return QString();
    }
    QDataStream ifs(&file);
    if (mudlet::scmRunTimeQtVersion >= QVersionNumber(5, 13, 0)) {
        ifs.setVersion(mudlet::scmQDataStreamFormat_5_12);
    }
    ifs >> previousVersion;
    file.close();
    file.remove();

    return previousVersion;
}
