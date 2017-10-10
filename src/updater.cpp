#include "updater.h"
#include "../3rdparty/dblsqd/feed.h"
#include "../3rdparty/dblsqd/update_dialog.h"

#include "pre_guard.h"
#include <QtConcurrent>
#include "post_guard.h"

Updater::Updater(QObject *parent) : QObject(parent)
{


}

void doUpdates() {
   silentlyUpdate();

}

void silentlyUpdate() {
        auto devSuffix = QByteArray(APP_BUILD).trimmed();

        // only update release builds
        if (!devSuffix.isEmpty()) {
            return;
        }

        auto feed = new dblsqd::Feed("https://feeds.dblsqd.com/MKMMR7HNSP65PquQQbiDIw", "release");
        auto updateDialog = new dblsqd::UpdateDialog(feed);
        updateDialog->showIfUpdatesAvailable();


        QObject::connect(feed, &dblsqd::Feed::ready, [=]() {
            qDebug() << "feed ready!" << feed->getUpdates().size() << "update available";

            feed->downloadRelease(feed->getUpdates().first());
        });


        QObject::connect(feed, &dblsqd::Feed::downloadFinished, [=]() {
            auto file = feed->getDownloadFile();

            QFuture<void> future = QtConcurrent::run(
                    [=](const QString& fileName) {
                        QProcess tar;
                        tar.setProcessChannelMode(QProcess::MergedChannels);
                        // we can assume tar to be present on a Linux system. If it's not, it'd be rather broken.
                        tar.start("tar",
                                  QStringList() << "-xvf" << fileName << "-C"
                                                << "/tmp/");
                        if (!tar.waitForFinished()) {
                            qDebug() << "Untarring" << fileName << "failed:" << tar.errorString();
                        }
                    },
                    file->fileName());


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
