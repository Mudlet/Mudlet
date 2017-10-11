#include "updater.h"
#include "../3rdparty/dblsqd/feed.h"
#include "../3rdparty/dblsqd/update_dialog.h"

#include "pre_guard.h"
#include <QtConcurrent>
#include "post_guard.h"

Updater::Updater(QObject* parent, bool mNoAutomaticUpdates) : QObject(parent)
{
    this->mNoAutomaticUpdates = mNoAutomaticUpdates;
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
    updateDialog = new dblsqd::UpdateDialog(feed, dblsqd::UpdateDialog::Manual);

    if (mNoAutomaticUpdates) {
        updateDialog->showIfUpdatesAvailable();
        return;
    }

    silentlyUpdate();
}

void Updater::silentlyUpdate() const
{
    QObject::connect(feed, &dblsqd::Feed::ready, [=]() {
        qDebug() << "Updates feed ready!" << feed->getUpdates().size() << "update available";
        feed->downloadRelease(feed->getUpdates().first());
    });


    QObject::connect(feed, &dblsqd::Feed::downloadFinished, [=]() {
        auto file = feed->getDownloadFile();

        QFuture<void> future = QtConcurrent::run([=](const QString& fileName) { untarOnLinux(fileName); }, file->fileName());


        // replace current binary with the unzipped one
        auto watcher = new QFutureWatcher<void>;
        QObject::connect(watcher, &QFutureWatcher<void>::finished, [=]() {
            // FIXME don't hardcode name in case we want to change it
            QFileInfo unzippedBinary("/tmp/Mudlet.AppImage");
            QString currentBinaryPath(QCoreApplication::applicationFilePath());

            auto executablePermissions = unzippedBinary.permissions();
            executablePermissions |= QFileDevice::ExeOwner | QFileDevice::ExeUser;

            QDir dir;
            if (!(dir.remove(currentBinaryPath) && QDir().rename(unzippedBinary.filePath(), currentBinaryPath))) {
                qDebug() << "updating" << currentBinaryPath << "with new version from" << unzippedBinary.filePath() << "failed";
                return;
            }

            QFile updatedBinary(QCoreApplication::applicationFilePath());
            if (!updatedBinary.setPermissions(executablePermissions)) {
                qDebug() << "couldn't executable permissions on updated Mudlet binary at" << QCoreApplication::applicationFilePath();
                return;
            }

            qDebug() << "Successfully updated Mudlet to" << feed->getUpdates().first().getVersion();

        });
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
    }
}
