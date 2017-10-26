#include "updater.h"
#include "mudlet.h"

#ifdef Q_OS_MACOS
#include "../3rdparty/sparkle-glue/CocoaInitializer.h"
#include "../3rdparty/sparkle-glue/SparkleAutoUpdater.h"
#endif

#include "pre_guard.h"
#include <QtConcurrent>
#include <QDateTime>
#include "post_guard.h"

Updater::Updater(QObject* parent) : QObject(parent), mUpdateInstalled(false)
{
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

#ifdef Q_OS_MACOS
    setupOnMacOS();
#endif
#ifdef Q_OS_LINUX
    setupOnLinux();
#endif
}

#ifdef Q_OS_MACOS
void Updater::setupOnMacOS()
{
    CocoaInitializer initializer;
    if (mudlet::self()->updateAutomatically()) {
        updater = new SparkleAutoUpdater("https://feeds.dblsqd.com/MKMMR7HNSP65PquQQbiDIw/release/mac/x86_64/appcast");
        updater->checkForUpdates();
    }
}
#endif

void Updater::setupOnLinux()
{
    feed = new dblsqd::Feed("https://feeds.dblsqd.com/MKMMR7HNSP65PquQQbiDIw", "release");

    QObject::connect(feed, &dblsqd::Feed::ready, [=]() { qDebug() << "Checked for updates:" << feed->getUpdates().size() << "update(s) available"; });

    QObject::connect(feed, &dblsqd::Feed::ready, [=]() {
        if (!mudlet::self()->updateAutomatically()) {
            return;
        }

        auto updates = feed->getUpdates();
        if (updates.isEmpty()) {
            return;
        }
        feed->downloadRelease(updates.first());
    });

    QObject::connect(feed, &dblsqd::Feed::downloadFinished, [=]() { qDebug() << "downloadFinished"; });

    // constructing the UpdateDialog triggers the update check
    updateDialog = new dblsqd::UpdateDialog(feed, mudlet::self()->updateAutomatically() ? dblsqd::UpdateDialog::Manual : dblsqd::UpdateDialog::OnLastWindowClosed);
    installButton = new QPushButton(tr("Update"));
    updateDialog->addInstallButton(installButton);
    connect(updateDialog, &dblsqd::UpdateDialog::installButtonClicked, this, &Updater::installButtonClicked);
}

void Updater::untarOnLinux(const QString& fileName) const
{
    QProcess tar;
    tar.setProcessChannelMode(QProcess::MergedChannels);
    // we can assume tar to be present on a Linux system. If it's not, it'd be rather broken.
    // tar output folder has to end with a slash
    tar.start("tar", QStringList() << "-xvf" << fileName << "-C" << QStandardPaths::writableLocation(QStandardPaths::TempLocation) + QStringLiteral("/"));
    if (!tar.waitForFinished()) {
        qDebug() << "Untarring" << fileName << "failed:" << tar.errorString();
    } else {
        qDebug() << "Tar output:" << tar.readAll().trimmed();
    }
}

void Updater::updateBinaryOnLinux()
{ // FIXME don't hardcode name in case we want to change it
    QFileInfo unzippedBinary(QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/Mudlet.AppImage");
    QString installedBinaryPath(QCoreApplication::applicationFilePath());

    auto executablePermissions = unzippedBinary.permissions();
    executablePermissions |= QFileDevice::ExeOwner | QFileDevice::ExeUser;

    QDir dir;
    // dir.rename actually moves a file
    if (!(dir.remove(installedBinaryPath) && dir.rename(unzippedBinary.filePath(), installedBinaryPath))) {
        qDebug() << "updating" << installedBinaryPath << "with new version from" << unzippedBinary.filePath() << "failed";
        return;
    }

    QFile updatedBinary(QCoreApplication::applicationFilePath());
    if (!updatedBinary.setPermissions(executablePermissions)) {
        qDebug() << "couldn't executable permissions on updated Mudlet binary at" << installedBinaryPath;
        return;
    }

    qDebug() << "Successfully updated Mudlet to" << feed->getUpdates().first().getVersion();
    mUpdateInstalled = true;
    installButton->setText(tr("Restart to apply update"));
    installButton->setEnabled(true);
    writeUpdateNote();
    emit updateInstalled();
}

void Updater::manuallyCheckUpdates()
{
    updateDialog->show();
}

void Updater::installButtonClicked(QAbstractButton* button, QString filePath)
{
    // if the update is already installed, then the button says 'Restart' - do so
    // FIXME check this for manual update from the menu
    if (mUpdateInstalled) {
        updateDialog->close();
        QProcess::startDetached(qApp->arguments()[0], qApp->arguments());
        return;
    }

    QFuture<void> future = QtConcurrent::run(this, &Updater::untarOnLinux, filePath);

    // replace current binary with the unzipped one
    auto watcher = new QFutureWatcher<void>;
    connect(watcher, &QFutureWatcher<void>::finished, this, &Updater::updateBinaryOnLinux);
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

void Updater::showChangelog() const {
    auto changelogDialog = new dblsqd::UpdateDialog(feed, dblsqd::UpdateDialog::ManualChangelog);
    changelogDialog->show();
}