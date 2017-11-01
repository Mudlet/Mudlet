#include "updater.h"
#include "mudlet.h"

#if defined(Q_OS_MACOS)
#include "../3rdparty/sparkle-glue/CocoaInitializer.h"
#include "../3rdparty/sparkle-glue/SparkleAutoUpdater.h"
#endif

#include "pre_guard.h"
#include <QtConcurrent>
#include "post_guard.h"

Updater::Updater(QObject* parent, QSettings* settings) : QObject(parent), mUpdateInstalled(false)
{
    Q_ASSERT_X(settings, "updater", "QSettings object is required for the updater to work");
    this->settings = settings;

#if defined(Q_OS_MACOS)
    CocoaInitializer initializer;
    msparkleUpdater = new SparkleAutoUpdater(QStringLiteral("https://feeds.dblsqd.com/MKMMR7HNSP65PquQQbiDIw/release/mac/x86_64/appcast"));
#endif
}

// start the update process and figure out what needs to be done
// if it's silent updates, do that right away, otherwise
// setup manual updates to do our custom actions
void Updater::checkUpdatesOnStart()
{
    // only update release builds to prevent auto-update from overwriting your
    // compiled binary while in development
    if (mudlet::self()->onDevelopmentVersion()) {
        //        return;
    }

#if defined(Q_OS_MACOS)
    setupOnMacOS();
#elif defined(Q_OS_LINUX)
    setupOnLinux();
#endif
}

#if defined(Q_OS_MACOS)
void Updater::setupOnMacOS()
{
// Sparkle should do this on its own
//    if (!updateAutomatically()) {
//        return;
//    }
//
//    msparkleUpdater->checkForUpdates();
}
#endif

#if defined(Q_OS_LINUX)
void Updater::setupOnLinux()
{
    feed = new dblsqd::Feed("https://feeds.dblsqd.com/MKMMR7HNSP65PquQQbiDIw", "release");

    QObject::connect(feed, &dblsqd::Feed::ready, [=]() { qWarning() << "Checked for updates:" << feed->getUpdates().size() << "update(s) available"; });

    QObject::connect(feed, &dblsqd::Feed::ready, [=]() {
        if (!updateAutomatically()) {
            return;
        }

        auto updates = feed->getUpdates();
        if (updates.isEmpty()) {
            return;
        }
        feed->downloadRelease(updates.first());
    });

    QObject::connect(feed, &dblsqd::Feed::downloadFinished, [=]() {
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

    // constructing the UpdateDialog triggers the update check
    updateDialog = new dblsqd::UpdateDialog(feed, updateAutomatically() ? dblsqd::UpdateDialog::Manual : dblsqd::UpdateDialog::OnLastWindowClosed, nullptr, settings);
    installOrRestartButton = new QPushButton(tr("Update"));
    updateDialog->addInstallButton(installOrRestartButton);
    connect(updateDialog, &dblsqd::UpdateDialog::installButtonClicked, this, &Updater::installOrRestartClicked);
}

void Updater::untarOnLinux(const QString& fileName) const
{
    Q_ASSERT_X(QThread::currentThread() != QCoreApplication::instance()->thread(), "untarOnLinux", "method should not be called in the main GUI thread to avoid a degradation in UX");

    QProcess tar;
    tar.setProcessChannelMode(QProcess::MergedChannels);
    // we can assume tar to be present on a Linux system. If it's not, it'd be rather broken.
    // tar output folder has to end with a slash
    tar.start("tar", QStringList() << "-xvf" << fileName << "-C" << QStandardPaths::writableLocation(QStandardPaths::TempLocation) + QStringLiteral("/"));
    if (!tar.waitForFinished()) {
        qWarning() << "Untarring" << fileName << "failed:" << tar.errorString();
    } else {
        qDebug() << "Tar output:" << tar.readAll().trimmed();
    }
}

void Updater::updateBinaryOnLinux()
{
    // FIXME don't hardcode name in case we want to change it
    QFileInfo unzippedBinary(QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/Mudlet.AppImage");
    QString installedBinaryPath(QCoreApplication::applicationFilePath());

    auto executablePermissions = unzippedBinary.permissions();
    executablePermissions |= QFileDevice::ExeOwner | QFileDevice::ExeUser;

    QDir dir;
    // dir.rename actually moves a file
    if (!(dir.remove(installedBinaryPath) && dir.rename(unzippedBinary.filePath(), installedBinaryPath))) {
        qWarning() << "updating" << installedBinaryPath << "with new version from" << unzippedBinary.filePath() << "failed";
        return;
    }

    QFile updatedBinary(QCoreApplication::applicationFilePath());
    if (!updatedBinary.setPermissions(executablePermissions)) {
        qWarning() << "couldn't set executable permissions on updated Mudlet binary at" << installedBinaryPath;
        return;
    }

    qWarning() << "Successfully updated Mudlet to" << feed->getUpdates().first().getVersion();
    writeUpdateNote();
    mUpdateInstalled = true;
    emit updateInstalled();
}

void Updater::installOrRestartClicked(QAbstractButton* button, QString filePath)
{
    // if the update is already installed, then the button says 'Restart' - do so
    if (mUpdateInstalled) {
        // timer is necessary as calling close right way doesn't seem to do the trick
        QTimer::singleShot(0, [=]() {
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
    QFuture<void> future = QtConcurrent::run(this, &Updater::untarOnLinux, filePath);

    // replace current binary with the unzipped one
    auto watcher = new QFutureWatcher<void>;
    connect(watcher, &QFutureWatcher<void>::finished, this, [=]() {
        updateBinaryOnLinux();
        installOrRestartButton->setText(tr("Restart to apply update"));
        installOrRestartButton->setEnabled(true);
    });
    watcher->setFuture(future);
}

// records a unix epoch on disk indicating that an update has happened.
// Mudlet will use that on the next launch to decide whenever it should show
// the window with the new features. The idea is that if you manually update (thus see the
// changelog already) and restart, you shouldn't see it again, and if you automatically
// updated, then you do want to see the changelog.
void Updater::writeUpdateNote() const
{
    QFile file(mudlet::getMudletPath(mudlet::mainDataItemPath, QStringLiteral("mudlet_updated_at")));
    bool opened = file.open(QIODevice::WriteOnly);
    if (!opened) {
        qWarning() << "Couldn't open update timestamp file for writing.";
        return;
    }

    QDataStream ifs(&file);
    ifs << QDateTime::currentDateTime().toMSecsSinceEpoch();
    file.close();
}

void Updater::showChangelog() const
{
    auto changelogDialog = new dblsqd::UpdateDialog(feed, dblsqd::UpdateDialog::ManualChangelog);
    changelogDialog->show();
}
#endif

void Updater::setAutomaticUpdates(const bool state)
{
#if defined(Q_OS_LINUX)
    return settings->setValue(QStringLiteral("DBLSQD/autoDownload"), state);
#elif defined(Q_OS_MACOS)
    msparkleUpdater->setAutomaticallyDownloadsUpdates(state);
#endif
}

bool Updater::updateAutomatically() const
{
#if defined(Q_OS_LINUX)
    return settings->value(QStringLiteral("DBLSQD/autoDownload"), true).toBool();
#elif defined(Q_OS_MACOS)
    return msparkleUpdater->automaticallyDownloadsUpdates();
#endif
}

void Updater::manuallyCheckUpdates()
{
#if defined(Q_OS_LINUX)
    updateDialog->show();
#elif defined(Q_OS_MACOS)
    msparkleUpdater->checkForUpdates();
#endif
}
