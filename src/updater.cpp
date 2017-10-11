#include "updater.h"
#include "../3rdparty/dblsqd/feed.h"
#include "../3rdparty/dblsqd/update_dialog.h"

#include "pre_guard.h"
#include <QtConcurrent>
#include "post_guard.h"

Updater::Updater(QObject* parent, bool mautomaticUpdates) : QObject(parent)
{
    this->mautomaticUpdates = mautomaticUpdates;
}

// start the update process and figure out what needs to be done
// if it's silent updates, do that, otherwise show the dialog
void Updater::doUpdates()
{
    auto devSuffix = QByteArray(APP_BUILD).trimmed();

    // only update release builds
    if (!devSuffix.isEmpty()) {
        //        return;
    }

    // constructing the UpdateDialog triggers the update check
    feed = new dblsqd::Feed("https://feeds.dblsqd.com/MKMMR7HNSP65PquQQbiDIw", "release");
    updateDialog = new dblsqd::UpdateDialog(feed, !mautomaticUpdates ? dblsqd::UpdateDialog::OnLastWindowClosed : dblsqd::UpdateDialog::Manual);

    QObject::connect(feed, &dblsqd::Feed::ready, [=]() { qDebug() << "Updates feed ready!" << feed->getUpdates().size() << "update(s) available"; });

    if (mautomaticUpdates) {
        silentlyUpdate();
    }
}

void Updater::silentlyUpdate() const
{
    QObject::connect(feed, &dblsqd::Feed::ready, [=]() { feed->downloadRelease(feed->getUpdates().first()); });

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
    tar.start("tar",
              QStringList() << "-xvf" << fileName << "-C"
                            << "/tmp/");
    if (!tar.waitForFinished()) {
        qDebug() << "Untarring" << fileName << "failed:" << tar.errorString();
    } else {
        qDebug() << "Tar output:" << tar.readAll().trimmed();
    }
}

void Updater::updateBinaryOnLinux() const
{ // FIXME don't hardcode name in case we want to change it
    QFileInfo unzippedBinary("/tmp/Mudlet.AppImage");
    QString installedBinaryPath(QCoreApplication::applicationFilePath());

    auto executablePermissions = unzippedBinary.permissions();
    executablePermissions |= QFileDevice::ExeOwner | QFileDevice::ExeUser;

    QDir dir;
    if (!(dir.remove(installedBinaryPath) && QDir().rename(unzippedBinary.filePath(), installedBinaryPath))) {
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
