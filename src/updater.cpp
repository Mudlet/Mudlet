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
#include "updater/Feed.h"
#include "updater/UpdateDialog.h"

#include <QDateTime>
#include <QMessageBox>
#include <QPushButton>
#include <QtConcurrent>
#include <chrono>
#include "../3rdparty/kdtoolbox/singleshot_connect/singleshot_connect.h"

using namespace std::chrono_literals;

#if defined(Q_OS_WINDOWS)
// Clean up legacy .nupkg files from the previous Squirrel/dblsqd update system's temp directory
static void cleanupSquirrelTempFiles()
{
    QString squirrelTempPath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + qsl("/SquirrelTemp");
    QDir squirrelTempDir(squirrelTempPath);

    if (!squirrelTempDir.exists()) {
        return;
    }

    qDebug() << "Cleaning up Mudlet files from SquirrelTemp:" << squirrelTempPath;

    QStringList filters;
    filters << qsl("Mudlet*.nupkg") << qsl("mudlet*.nupkg");
    QFileInfoList nupkgFiles = squirrelTempDir.entryInfoList(filters, QDir::Files);

    int removedCount = 0;
    qint64 freedSpace = 0;

    for (const QFileInfo& fileInfo : nupkgFiles) {
        qint64 fileSize = fileInfo.size();
        if (QFile::remove(fileInfo.absoluteFilePath())) {
            removedCount++;
            freedSpace += fileSize;
            qDebug() << "Removed:" << fileInfo.fileName() << "(" << (fileSize / 1024 / 1024) << "MB)";
        } else {
            qWarning() << "Failed to remove:" << fileInfo.absoluteFilePath();
        }
    }

    if (removedCount > 0) {
        qDebug() << "Cleaned up" << removedCount << "Mudlet .nupkg files from SquirrelTemp, freed" << (freedSpace / 1024 / 1024) << "MB of disk space";
    }
}
#endif // Q_OS_WINDOWS

// update flows:
// linux: new AppImage is downloaded, extracted from its tar archive, and put in place of the old one
//   user then only restarts mudlet to get the new version
// windows: installer .exe is downloaded from GitHub Releases. When the user clicks restart,
//   a batch file is created that waits for Mudlet to exit, then runs the installer
// mac: handled completely outside of Mudlet by Sparkle

Updater::Updater(QObject* parent, QSettings* settings, bool testVersion)
: QObject(parent)
#if !defined(Q_OS_MACOS)
//: Label for the update/restart button in the main toolbar
, mpInstallOrRestart(new QPushButton(tr("Update")))
#endif
, mUpdateInstalled(false)
{
    Q_ASSERT_X(settings, "updater", "QSettings object is required for the updater to work");
    mSettings = settings;

    feed.reset(new dblsqd::Feed(this));
    feed->setRepo(qsl("Mudlet"), qsl("Mudlet"), testVersion);
    mPeriodicCheck = std::make_unique<QTimer>();
}

Updater::~Updater()
{
#if !defined(Q_OS_MACOS)
    // QPointer::data() returns null if Qt already deleted the dialog; only
    // delete if it hasn't been cleaned up yet.
    if (updateDialog) {
        delete updateDialog;
    }
#endif
}

void Updater::checkUpdatesOnStart()
{
#if defined(Q_OS_MACOS)
    setupOnMacOS();
#elif defined(Q_OS_LINUX)
    setupOnLinux();
#elif defined(Q_OS_WINDOWS)
    setupOnWindows();
#endif

    mPeriodicCheck->setInterval(12h);
    connect(mPeriodicCheck.get(), &QTimer::timeout, this, [this] {
        KDToolBox::connectSingleShot(feed.get(), &dblsqd::Feed::ready, this, [this]() {
            auto updates = feed->getUpdates(dblsqd::Release::getCurrentRelease());
            qWarning() << "Twice-daily check for updates:" << updates.size() << "update(s) available";
            if (updates.isEmpty()) {
                return;
            }

            if (!updateAutomatically()) {
                emit signal_updateAvailable(updates.size());
                return;
            }

            if (!downloadReleaseIfValid(updates.first())) {
                emit signal_updateAvailable(updates.size());
            }
        });
        KDToolBox::connectSingleShot(feed.get(), &dblsqd::Feed::loadError, this, [](const QString& error) {
            qWarning() << "Twice-daily update check: failed to load feed:" << error;
        });
        feed->load();
    });
    mPeriodicCheck->start();
}

void Updater::setAutomaticUpdates(const bool state)
{
#if defined(Q_OS_MACOS)
    msparkleUpdater->setAutomaticallyDownloadsUpdates(state);
#else
    dblsqd::UpdateDialog::enableAutoDownload(state, mSettings);
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
    return dblsqd::UpdateDialog::autoDownloadEnabled(true, mSettings);
#endif
}

void Updater::manuallyCheckUpdates()
{
#if defined(Q_OS_MACOS)
    msparkleUpdater->checkForUpdates();
#else
    if (mManualCheckInProgress) {
        return;
    }
    mManualCheckInProgress = true;

    feed->load();
    KDToolBox::connectSingleShot(feed.get(), &dblsqd::Feed::ready, this, [this]() {
        mManualCheckInProgress = false;
        showDialogManually();
    });
    KDToolBox::connectSingleShot(feed.get(), &dblsqd::Feed::loadError, this, [this](const QString& error) {
        mManualCheckInProgress = false;
        emit signal_updateCheckFailed(error);
    });
#endif
}

void Updater::showDialogManually() const
{
    if (!updateDialog) {
        qWarning() << "showDialogManually called but update dialog not initialized";
        return;
    }
    updateDialog->show();
}

void Updater::showChangelog() const
{
    auto changelogDialog = new dblsqd::UpdateDialog(feed.get(), dblsqd::UpdateDialog::ManualChangelog, mSettings);
    changelogDialog->setAttribute(Qt::WA_DeleteOnClose);
    changelogDialog->setPreviousVersion(getPreviousVersion());
    changelogDialog->show();
}

void Updater::showFullChangelog() const
{
    if (!feed->isReady()) {
        KDToolBox::connectSingleShot(feed.get(), &dblsqd::Feed::ready, feed.get(), [=, this]() {
            showFullChangelog();
        });
        KDToolBox::connectSingleShot(feed.get(), &dblsqd::Feed::loadError, feed.get(), [](const QString& error) {
            qWarning() << "Failed to load feed for changelog:" << error;
            //: Error title for dialog shown when changelog fails to load
            QMessageBox::warning(nullptr,
                                 tr("Changelog Error"),
                                 //: Error message shown when changelog fails to load from the server
                                 tr("Could not load the changelog. Please try again later."));
        });
        feed->load();
        return;
    }

    auto changelogDialog = new dblsqd::UpdateDialog(feed.get(), dblsqd::UpdateDialog::ManualChangelog, mSettings);
    changelogDialog->setAttribute(Qt::WA_DeleteOnClose);
    auto releases = feed->getReleases();
    if (!releases.isEmpty()) {
        changelogDialog->setMinVersion(releases.constLast().getVersion());
    }
    changelogDialog->setMaxVersion(QApplication::applicationVersion());
    changelogDialog->show();
}

bool Updater::downloadReleaseIfValid(const dblsqd::Release& release)
{
    const QUrl downloadUrl = release.getDownloadUrl();
    if (!downloadUrl.isValid() || downloadUrl.isEmpty()) {
        qWarning() << "Update check: invalid download URL for release" << release.getVersion();
        if (mManualCheckInProgress) {
            //: Error shown when no download is available for the user's platform. %1 is the version number.
            emit signal_updateCheckFailed(tr("No download available for version %1. Please try again later or download manually from https://www.mudlet.org/download/").arg(release.getVersion()));
        }
        return false;
    }
    feed->downloadRelease(release);
    return true;
}

void Updater::finishSetup()
{
    auto updates = feed->getUpdates(dblsqd::Release::getCurrentRelease());
#if defined(Q_OS_LINUX)
    if (!updates.isEmpty()) {
        qWarning() << "Successfully updated Mudlet to" << updates.constFirst().getVersion();
    } else {
        qWarning() << "Update finished but could not determine target version";
    }
#elif defined(Q_OS_WINDOWS)
    if (!updates.isEmpty()) {
        qWarning() << "Mudlet prepped to update to" << updates.first().getVersion() << "on restart";
    } else {
        qWarning() << "Mudlet prepped to update on restart";
    }
    cleanupSquirrelTempFiles();
#endif
    recordUpdateTime();
    recordUpdatedVersion();
    mUpdateInstalled = true;
    emit signal_updateInstalled();
}

#if defined(Q_OS_MACOS)
void Updater::setupOnMacOS()
{
    // don't need to explicitly check for updates - sparkle will do so on its own
    msparkleUpdater = new SparkleUpdater(this);
}
#endif // Q_OS_MACOS

#if !defined(Q_OS_MACOS)
void Updater::setupPlatformUpdater()
{
    // Setup to automatically download the new release when an update is available
    connect(feed.get(), &dblsqd::Feed::ready, this, [=, this]() {
        auto* pMudlet = mudlet::self();
        if (!pMudlet || pMudlet->developmentVersion) {
            return;
        }

        auto updates = feed->getUpdates(dblsqd::Release::getCurrentRelease());
        qWarning() << "Checked for updates:" << updates.size() << "update(s) available";
        if (updates.isEmpty()) {
            return;
        }
        emit signal_updateAvailable(updates.size());
    });

    connect(feed.get(), &dblsqd::Feed::downloadError, this, [this](const QString& error) {
        qWarning() << "Automatic update download failed:" << error;
        emit signal_updateCheckFailed(error);
    });
}
#endif // !Q_OS_MACOS

#if defined(Q_OS_WINDOWS)
void Updater::setupOnWindows()
{
    cleanupSquirrelTempFiles();
    setupPlatformUpdater();

    // Setup to run setup.exe to replace the old installation
    connect(feed.get(), &dblsqd::Feed::downloadFinished, this, [=, this]() {
        // if automatic updates are enabled, and this isn't a manual check, perform the automatic update
        if (!(updateAutomatically() && updateDialog && updateDialog->isHidden())) {
            return;
        }

        const QString fileName = feed->getDownloadFilePath();
        if (fileName.isEmpty()) {
            qWarning() << "Download finished but no download file available - feed URL:" << feed->getUrl();
            //: Error shown when the automatic update download finished but produced no file
            emit signal_updateCheckFailed(tr("Update download failed. Please try again or download manually from https://www.mudlet.org/download/"));
            return;
        }

        QFuture<void> future = QtConcurrent::run([=, this]() {
            prepareSetupOnWindows(fileName);
        });

        auto watcher = new QFutureWatcher<void>;
        connect(watcher, &QFutureWatcher<void>::finished, this, &Updater::finishSetup);
        connect(watcher, &QFutureWatcher<void>::finished, watcher, &QObject::deleteLater);
        watcher->setFuture(future);
    });

    // finally, create the dblsqd objects. Constructing the UpdateDialog triggers the update check
    updateDialog = new dblsqd::UpdateDialog(feed.get(), updateAutomatically() ? dblsqd::UpdateDialog::OnLastWindowClosed : dblsqd::UpdateDialog::Manual, mSettings);
    //: Label for the update button shown in the update dialog
    mpInstallOrRestart->setText(tr("Update"));
    updateDialog->addInstallButton(mpInstallOrRestart);
    connect(updateDialog, &dblsqd::UpdateDialog::installButtonClicked, this, &Updater::slot_installOrRestartClicked);
}

void Updater::prepareSetupOnWindows(const QString& downloadedSetupName)
{
    mDownloadedInstallerPath = downloadedSetupName;
    qWarning() << "Installer ready at:" << mDownloadedInstallerPath;
}
#endif // Q_OS_WINDOWS

#if defined(Q_OS_LINUX)
void Updater::setupOnLinux()
{
    setupPlatformUpdater();

    // Setup to unzip and replace old binary when the download is done
    connect(feed.get(), &dblsqd::Feed::downloadFinished, this, [=, this]() {
        // if automatic updates are enabled, and this isn't a manual check, perform the automatic update
        if (!(updateAutomatically() && updateDialog && updateDialog->isHidden())) {
            return;
        }

        const QString fileName = feed->getDownloadFilePath();
        if (fileName.isEmpty()) {
            qWarning() << "Download finished but no download file available - feed URL:" << feed->getUrl();
            //: Error shown when the automatic update download finished but produced no file
            emit signal_updateCheckFailed(tr("Update download failed. Please try again or download manually from https://www.mudlet.org/download/"));
            return;
        }

        QFuture<void> future = QtConcurrent::run([=, this]() {
            untarOnLinux(fileName);
        });

        auto watcher = new QFutureWatcher<void>;
        connect(watcher, &QFutureWatcher<void>::finished, this, &Updater::slot_updateLinuxBinary);
        connect(watcher, &QFutureWatcher<void>::finished, watcher, &QObject::deleteLater);
        watcher->setFuture(future);
    });

    // finally, create the dblsqd objects. Constructing the UpdateDialog triggers the update check
    updateDialog = new dblsqd::UpdateDialog(feed.get(), updateAutomatically() ? dblsqd::UpdateDialog::OnLastWindowClosed : dblsqd::UpdateDialog::Manual, mSettings);
    //: Label for the update button shown in the update dialog
    mpInstallOrRestart->setText(tr("Update"));
    updateDialog->addInstallButton(mpInstallOrRestart);
    connect(updateDialog, &dblsqd::UpdateDialog::installButtonClicked, this, &Updater::slot_installOrRestartClicked);
}

void Updater::untarOnLinux(const QString& fileName)
{
    mUnzippedBinaryName.clear();
    Q_ASSERT_X(QThread::currentThread() != QCoreApplication::instance()->thread(), "untarOnLinux", "method should not be called in the main GUI thread to avoid a degradation in UX");
    qWarning() << __func__ << "started";

    QProcess tar;
    tar.setProcessChannelMode(QProcess::MergedChannels);
    // we can assume tar to be present on a Linux system. If it's not, it'd be rather broken.
    // tar output folder has to end with a slash
    tar.start(qsl("tar"), QStringList() << qsl("-xvf") << fileName << qsl("-C") << QStandardPaths::writableLocation(QStandardPaths::TempLocation) + qsl("/"));
    if (!tar.waitForStarted(5000)) {
        qWarning() << "Could not start tar:" << tar.errorString();
    } else if (!tar.waitForFinished(300000)) {
        tar.kill();
        qWarning() << "Untarring" << fileName << "timed out after 5 minutes:" << tar.errorString();
    } else if (tar.exitCode() != 0) {
        qWarning() << "Untarring" << fileName << "failed - exit code:" << tar.exitCode() << tar.errorString();
    } else {
        const QString output = tar.readAll().trimmed();
        if (output.isEmpty() || output.contains(QLatin1Char('\n'))) {
            qWarning() << "Unexpected tar output (expected single filename):" << output;
        } else {
            mUnzippedBinaryName = output;
        }
    }
    qWarning() << __func__ << "finished";
}

void Updater::slot_updateLinuxBinary()
{
    qWarning() << __func__ << "started";

    if (mUnzippedBinaryName.isEmpty()) {
        qWarning() << "Extraction failed - no binary to install, aborting update";
        //: Error shown when extracting the downloaded update archive fails on Linux
        emit signal_updateCheckFailed(tr("Failed to extract the update. Please try again or download manually from https://www.mudlet.org/download/"));
        return;
    }

    QFileInfo unzippedBinary(QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/" + mUnzippedBinaryName);
    auto systemEnvironment = QProcessEnvironment::systemEnvironment();
    auto appimageLocation = systemEnvironment.contains(qsl("APPIMAGE")) ? systemEnvironment.value(qsl("APPIMAGE"), QString()) : QCoreApplication::applicationFilePath();

    const QString& installedBinaryPath(appimageLocation);

    auto executablePermissions = unzippedBinary.permissions();
    executablePermissions |= QFileDevice::ExeOwner | QFileDevice::ExeUser;

    QDir dir;
    // Safely replace the old binary: rename old to backup first so we can
    // restore it if placing the new binary fails (e.g. cross-device rename)
    const QString backupPath = installedBinaryPath + qsl(".bak");
    if (!dir.remove(backupPath) && QFile::exists(backupPath)) {
        qWarning() << "Could not remove stale backup at" << backupPath;
        //: Error shown when the automatic update fails to install on Linux
        emit signal_updateCheckFailed(tr("Failed to install the update. Please try again or download manually from https://www.mudlet.org/download/"));
        return;
    }
    if (!dir.rename(installedBinaryPath, backupPath)) {
        qWarning() << "could not back up old binary from" << installedBinaryPath << "to" << backupPath;
        //: Error shown when the automatic update fails to install on Linux
        emit signal_updateCheckFailed(tr("Failed to install the update. Please try again or download manually from https://www.mudlet.org/download/"));
        return;
    }
    if (!dir.rename(unzippedBinary.filePath(), installedBinaryPath)) {
        qWarning() << "could not move new binary from" << unzippedBinary.filePath() << "to" << installedBinaryPath << "- restoring backup";
        if (!dir.rename(backupPath, installedBinaryPath)) {
            qWarning() << "could not restore backup from" << backupPath << "to" << installedBinaryPath;
            //: Error shown when the update fails and the previous version could not be restored automatically. %1 is the file path to the backup copy.
            emit signal_updateCheckFailed(tr("Failed to install the update and could not restore the previous version. "
                                             "Your previous version is saved at: %1 - please rename it back manually. "
                                             "Alternatively, download a fresh copy from https://www.mudlet.org/download/")
                                                  .arg(backupPath));
        } else {
            //: Error shown when the automatic update fails to install on Linux
            emit signal_updateCheckFailed(tr("Failed to install the update. Please try again or download manually from https://www.mudlet.org/download/"));
        }
        return;
    }
    if (!dir.remove(backupPath)) {
        qWarning() << "Could not clean up backup file:" << backupPath;
    }
    qWarning() << "successfully replaced old binary with new binary";

    QFile updatedBinary(appimageLocation);
    if (!updatedBinary.setPermissions(executablePermissions)) {
        qWarning() << "couldn't set executable permissions on updated Mudlet binary at" << installedBinaryPath;
        //: Error shown when the automatic update fails to install on Linux
        emit signal_updateCheckFailed(tr("Failed to install the update. Please try again or download manually from https://www.mudlet.org/download/"));
        return;
    }
    qWarning() << "successfully set executable permissions for the new binary";

    finishSetup();
    qWarning() << __func__ << "finished";
}
#endif // Q_OS_LINUX

void Updater::slot_installOrRestartClicked(QAbstractButton* button, const QString& filePath)
{
    Q_UNUSED(button)

    // moc on macOS requires this function definition to exist even though macOS uses Sparkle instead
#if !defined(Q_OS_MACOS)

    // if the update is already installed, then the button says 'Restart' - do so
    if (mUpdateInstalled) {
        // defer to next event loop iteration so the dialog close happens after the button click handler returns
        QTimer::singleShot(0, this, [=, this]() {
            updateDialog->close();
            updateDialog->done(0);
        });

#if defined(Q_OS_WINDOWS)
        // On Windows, create and launch a batch file that waits for Mudlet to exit,
        // then runs the installer. This prevents "file in use" errors during the update.
        //: Error title for update-related warning dialogs
        const QString errorTitle = tr("Update Error");

        if (mDownloadedInstallerPath.isEmpty() || !QFile::exists(mDownloadedInstallerPath)) {
            qWarning() << "Installer not found at:" << mDownloadedInstallerPath;
            //: Error shown when the downloaded installer file cannot be found on disk
            QMessageBox::warning(nullptr, errorTitle, tr("The update installer could not be found. Please try checking for updates again."));
            return;
        }

        // Copy the installer to a permanent location with a known name. We copy
        // (not move) because AV software may still have a lock on the file, and
        // copy only needs read access.
        // Use a unique filename with timestamp to avoid conflicts with locked files.
        QString installerPath = qsl("%1/mudlet-setup-%2.exe").arg(QStandardPaths::writableLocation(QStandardPaths::TempLocation)).arg(QDateTime::currentSecsSinceEpoch());
        if (!QFile::copy(mDownloadedInstallerPath, installerPath)) {
            qWarning() << "Failed to copy installer from" << mDownloadedInstallerPath << "to" << installerPath;
            //: Error shown when the installer file cannot be copied to a temporary location for launch
            QMessageBox::warning(nullptr, errorTitle, tr("Could not prepare the update installer. Please try again or download the update manually from https://www.mudlet.org/download/"));
            return;
        }

        // Create a batch file that waits for Mudlet to exit before launching installer
        // this avoids shell quoting issues that happen with QProcess::startDetached
        QString batchPath = qsl("%1/mudlet-update.bat").arg(QStandardPaths::writableLocation(QStandardPaths::TempLocation));
        QFile batchFile(batchPath);
        if (batchFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QString exeName = QFileInfo(QCoreApplication::applicationFilePath()).fileName();
            // Uses ping for delay instead of timeout.exe because timeout doesn't work when stdin is redirected.
            // Change to temp directory immediately to release handle on Mudlet's app folder.
            QString batchContent = qsl("@echo off\r\n"
                                       "cd /d %TEMP%\r\n"
                                       "echo Mudlet updater: waiting for %1 to exit...\r\n"
                                       ":wait_mudlet\r\n"
                                       "tasklist /FI \"IMAGENAME eq %1\" 2>NUL | C:\\Windows\\System32\\find.exe /I \"%1\" >NUL\r\n"
                                       "if %ERRORLEVEL%==0 (\r\n"
                                       "    echo Mudlet updater: %1 still running, waiting...\r\n"
                                       "    ping -n 2 127.0.0.1 > nul\r\n"
                                       "    goto wait_mudlet\r\n"
                                       ")\r\n"
                                       "echo Mudlet updater: %1 exited, waiting for cleanup...\r\n"
                                       "ping -n 4 127.0.0.1 > nul\r\n"
                                       "echo Mudlet updater: launching installer...\r\n"
                                       "echo Mudlet updater: running %2\r\n"
                                       "\"%2\"\r\n"
                                       "echo Mudlet updater: installer finished with exit code %ERRORLEVEL%\r\n")
                                           .arg(exeName, QDir::toNativeSeparators(installerPath));
            if (batchFile.write(batchContent.toLocal8Bit()) == -1) {
                qWarning() << "Failed to write update batch file:" << batchFile.errorString();
                //: Error shown when the batch file for managing the update process cannot be written. %1 is the path to the installer.
                QMessageBox::warning(nullptr, errorTitle, tr("Could not prepare the update. Please close Mudlet and run the installer manually:\n%1").arg(QDir::toNativeSeparators(installerPath)));
                return;
            }
            batchFile.close();

            if (!QProcess::startDetached(batchPath, QStringList())) {
                qWarning() << "Failed to launch update batch file:" << batchPath;
                //: Error shown when the update installer process fails to start
                QMessageBox::warning(nullptr, errorTitle, tr("Could not launch the update installer. Please restart Mudlet and try again."));
                return;
            }
            qWarning() << "Launching installer via batch file:" << installerPath;
        } else {
            qWarning() << "Failed to create update batch file:" << batchFile.errorString();
            //: Error shown when the batch file for managing the update process cannot be created. %1 is the path to the installer.
            QMessageBox::warning(nullptr, errorTitle, tr("Could not prepare the update. Please close Mudlet and run the installer manually:\n%1").arg(QDir::toNativeSeparators(installerPath)));
            return;
        }

        if (mudlet::self()) {
            mudlet::self()->forceClose();
        }
        // Mudlet is not restarted here - the installer is expected to handle launching the updated version
        return;
#else
        if (mudlet::self()) {
            mudlet::self()->forceClose();
        }
        if (!QProcess::startDetached(qApp->arguments()[0], qApp->arguments())) {
            qWarning() << "Failed to restart Mudlet after update";
            //: Error title for dialog shown when Mudlet fails to restart after updating
            QMessageBox::critical(nullptr,
                                  tr("Update Error"),
                                  //: Error message shown when Mudlet fails to restart after updating on Linux
                                  tr("Could not restart Mudlet after the update. Please start it manually."));
        }
        return;
#endif
    }

#if defined(Q_OS_LINUX)
    QFuture<void> future = QtConcurrent::run([this, filePath]() {
        untarOnLinux(filePath);
    });
#elif defined(Q_OS_WINDOWS)
    QFuture<void> future = QtConcurrent::run([this, filePath]() {
        prepareSetupOnWindows(filePath);
    });
#endif

    auto watcher = new QFutureWatcher<void>;
    connect(watcher, &QFutureWatcher<void>::finished, this, [=, this]() {
#if defined(Q_OS_LINUX)
        slot_updateLinuxBinary();
#elif defined(Q_OS_WINDOWS)
        finishSetup();
#endif
        if (mUpdateInstalled) {
            //: Label for the button shown after the update has been downloaded and installed, prompting user to restart
            mpInstallOrRestart->setText(tr("Restart to apply update"));
        } else {
            //: Label for the update button shown when the update installation failed
            mpInstallOrRestart->setText(tr("Update failed"));
        }
        mpInstallOrRestart->setEnabled(true);
        watcher->deleteLater();
    });
    watcher->setFuture(future);
#endif // !Q_OS_MACOS
}

// Records a timestamp on disk so shouldShowChangelog() can detect automatic updates on next launch
void Updater::recordUpdateTime() const
{
    // The updater outlives the main window; without it there is no config
    // path to write the changelog marker to:
    if (!mudlet::self()) {
        return;
    }
    QSaveFile file(mudlet::getMudletPath(enums::mainDataItemPath, qsl("mudlet_updated_at")));
    bool opened = file.open(QIODevice::WriteOnly);
    if (!opened) {
        qWarning() << "Couldn't open update timestamp file for writing.";
        return;
    }

    QDataStream ofs(&file);
    if (mudlet::scmRunTimeQtVersion >= QVersionNumber(5, 13, 0)) {
        ofs.setVersion(mudlet::scmQDataStreamFormat_5_12);
    }
    ofs << QDateTime::currentDateTime().toMSecsSinceEpoch();
    if (!file.commit()) {
        qWarning() << "Updater::recordUpdateTime: error recording update time:" << file.errorString();
    }
}

// records the previous version of Mudlet that we updated from, so we can show
// the changelog on next startup for the latest version only
void Updater::recordUpdatedVersion() const
{
    // The updater outlives the main window; without it there is no config
    // path to write the changelog marker to:
    if (!mudlet::self()) {
        return;
    }
    QSaveFile file(mudlet::getMudletPath(enums::mainDataItemPath, qsl("mudlet_updated_from")));
    bool opened = file.open(QIODevice::WriteOnly);
    if (!opened) {
        qWarning() << "Couldn't open update version file for writing.";
        return;
    }

    QDataStream ofs(&file);
    if (mudlet::scmRunTimeQtVersion >= QVersionNumber(5, 13, 0)) {
        ofs.setVersion(mudlet::scmQDataStreamFormat_5_12);
    }
    // The full version (including any -ptb suffix) so shouldShowChangelog()
    // can tell whether the running version actually changed:
    ofs << QCoreApplication::applicationVersion();
    if (!file.commit()) {
        qWarning() << "Updater::recordUpdatedVersion: error saving old mudlet version:" << file.errorString();
    }
}

// Returns true if the changelog should be shown on this launch. Only applies to
// non-development builds with auto-updates on non-macOS (Sparkle handles its own changelog).
// Requires at least 5 minutes since the update to avoid re-showing a just-seen changelog.
bool Updater::shouldShowChangelog()
{
#if defined(Q_OS_MACOS)
    return false;
#endif

    if (mudlet::self()->developmentVersion || !updateAutomatically()) {
        return false;
    }

    QFile file(mudlet::self()->getMudletPath(enums::mainDataItemPath, qsl("mudlet_updated_at")));
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

    if (ifs.status() != QDataStream::Ok) {
        qWarning() << "Failed to read update timestamp file, treating as missing";
        file.remove();
        return false;
    }

    auto currentDateTime = QDateTime::currentDateTime().toMSecsSinceEpoch();
    auto minsSinceUpdate = (currentDateTime - updateTimestamp) / 1000 / 60;

    file.remove();

    // The markers are also written when an update was downloaded but never
    // installed (e.g. the user declined the restart). If the "updated from"
    // version is still the one running, no update actually happened - don't
    // show a changelog for it:
    if (readPreviousVersionFile(false) == QCoreApplication::applicationVersion()) {
        QFile::remove(mudlet::self()->getMudletPath(enums::mainDataItemPath, qsl("mudlet_updated_from")));
        return false;
    }

    return minsSinceUpdate >= 5;
}

QString Updater::getPreviousVersion() const
{
    return readPreviousVersionFile(true);
}

QString Updater::readPreviousVersionFile(const bool removeAfterRead) const
{
    QFile file(mudlet::self()->getMudletPath(enums::mainDataItemPath, qsl("mudlet_updated_from")));
    bool opened = file.open(QIODevice::ReadOnly);
    QString previousVersion;
    if (!opened) {
        if (removeAfterRead) {
            file.remove();
        }
        return QString();
    }
    QDataStream ifs(&file);
    if (mudlet::scmRunTimeQtVersion >= QVersionNumber(5, 13, 0)) {
        ifs.setVersion(mudlet::scmQDataStreamFormat_5_12);
    }
    ifs >> previousVersion;
    file.close();
    if (removeAfterRead) {
        file.remove();
    }

    if (ifs.status() != QDataStream::Ok) {
        qWarning() << "Failed to read previous version file, treating as missing";
        return QString();
    }

    return previousVersion;
}
