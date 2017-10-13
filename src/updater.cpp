#include "updater.h"
#include "../3rdparty/dblsqd/dblsqd/feed.h"
#include "../3rdparty/dblsqd/dblsqd/update_dialog.h"
#include "mudlet.h"

#include "pre_guard.h"
#include <QtConcurrent>
#include "post_guard.h"

Updater::Updater(QObject* parent) : QObject(parent)
{
}

// start the update process and figure out what needs to be done
// if it's silent updates, do that right away, otherwise
// setup manual updates to do our custom actions
void Updater::doUpdates()
{
    auto devSuffix = QByteArray(APP_BUILD).trimmed();

    // only update release builds
    if (!devSuffix.isEmpty()) {
        //        return;
    }

    // constructing the UpdateDialog triggers the update check
    feed = new dblsqd::Feed("https://feeds.dblsqd.com/MKMMR7HNSP65PquQQbiDIw", "release");
    updateDialog = new dblsqd::UpdateDialog(feed, mudlet::self()->updateAutomatically() ? dblsqd::UpdateDialog::Manual : dblsqd::UpdateDialog::OnLastWindowClosed);

    QObject::connect(feed, &dblsqd::Feed::ready, [=]() { qDebug() << "Updates feed ready!" << feed->getUpdates().size() << "update(s) available"; });

    setupEvents();
}


void Updater::setupEvents() const
{
    QObject::connect(feed, &dblsqd::Feed::ready, [=]() {
        if (!mudlet::self()->updateAutomatically()) {
            return;
        }

        feed->downloadRelease(feed->getUpdates().first());
    });

    QObject::connect(feed, &dblsqd::Feed::downloadFinished, [=]() {
        auto file = feed->getDownloadFile();

        QFuture<void> future = QtConcurrent::run(this, &Updater::untarOnLinux, file->fileName());

        // replace current binary with the unzipped one
        auto watcher = new QFutureWatcher<void>;
        connect(watcher, &QFutureWatcher<void>::finished, this, &Updater::updateBinaryOnLinux);
        watcher->setFuture(future);
    });
}

void Updater::untarOnLinux(const QString& fileName) const
{
    QProcess tar;
    tar.setProcessChannelMode(QProcess::MergedChannels);
    // we can assume tar to be present on a Linux system. If it's not, it'd be rather broken.
    tar.start("tar", QStringList() << "-xvf" << fileName << "-C" << QStandardPaths::writableLocation(QStandardPaths::TempLocation) << "/");
    if (!tar.waitForFinished()) {
        qDebug() << "Untarring" << fileName << "failed:" << tar.errorString();
    } else {
        qDebug() << "Tar output:" << tar.readAll().trimmed();
    }
}

void Updater::updateBinaryOnLinux() const
{ // FIXME don't hardcode name in case we want to change it
    QFileInfo unzippedBinary(QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/Mudlet.AppImage");
    QString installedBinaryPath(QCoreApplication::applicationFilePath());

    auto executablePermissions = unzippedBinary.permissions();
    executablePermissions |= QFileDevice::ExeOwner | QFileDevice::ExeUser;

    QDir dir;
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
}
