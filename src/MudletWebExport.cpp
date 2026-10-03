/***************************************************************************
 *   Copyright (C) 2026 by Vadim Peretokin - vadim.peretokin@mudlet.org    *
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

#include "MudletWebExport.h"

#include "Host.h"
#include "HostDialogs.h"
#include "MudletApp.h"
#include "TMap.h"
#include "TRoomDB.h"
#include "dlgTriggerEditor.h"
#include "utils.h"

#include <QBuffer>
#include <QDataStream>
#include <QDateTime>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QRegularExpression>
#include <QSet>
#include <QtConcurrentRun>

#include <zip.h>

const QString MudletWebExport::scmMudletWebUrl = qsl("https://web.mudlet.org/");

namespace {

// Folders of the profile that stay behind: older saves and maps (a fresh one of
// each goes in instead), logs and replays, and media the game sent, which it
// will send again.
bool leftBehind(const QString& relativePath)
{
    const QString top = relativePath.section(QLatin1Char('/'), 0, 0);
    if (relativePath == top) {
        // Only there when the player opted out of the system keychain - and a
        // file bound for a browser's downloads folder is no place for it.
        return top == qsl("password");
    }
    return top == qsl("current") || top == qsl("map") || top == qsl("log") || top == qsl("media");
}

bool isArchive(const QString& path)
{
    return path.endsWith(qsl(".mpackage"), Qt::CaseInsensitive) || path.endsWith(qsl(".zip"), Qt::CaseInsensitive);
}

} // namespace

MudletWebExport::MudletWebExport(Host* pHost, const QString& archivePathFileName, QObject* parent)
: QObject(parent)
, mpHost(pHost)
, mArchivePathFileName(archivePathFileName)
{
}

QString MudletWebExport::suggestedFileName(const QString& profileName)
{
    static const QRegularExpression unsafe(qsl(R"([\\/:*?"<>|])"));
    QString name = profileName;
    name.replace(unsafe, qsl("_"));
    return qsl("%1-mudlet-web.zip").arg(name);
}

void MudletWebExport::start()
{
    if (!mpHost) {
        finish(false, tr("The profile is no longer open."));
        return;
    }
    mHostGone = connect(mpHost, &QObject::destroyed, this, [this]() {
        finish(false, tr("The profile was closed before it could be exported."));
    });

    if (auto* dialogs = HostDialogs::find(mpHost); dialogs && dialogs->mpEditorDialog) {
        // An edit still sitting in the editor is the newest thing the player made
        dialogs->mpEditorDialog->slot_saveEdits();
    }
    saveThenWrite();
}

void MudletWebExport::saveThenWrite()
{
    if (!mpHost || mFinished) {
        return;
    }
    if (mpHost->currentlySavingProfile()) {
        // Ours has to start after it: saveProfile() refuses to run alongside it
        connect(mpHost, &Host::profileSaveFinished, this, &MudletWebExport::saveThenWrite, Qt::SingleShotConnection);
        return;
    }

    auto [ok, xmlPathFileName, error] = mpHost->saveProfile(QString(), QString(), true);
    if (!ok) {
        //: Shown when exporting a profile for Mudlet Web fails because the profile itself could not be saved first. %1 is the reason.
        finish(false, tr("The profile could not be saved: %1").arg(error));
        return;
    }
    if (!mpHost->currentlySavingProfile()) {
        writeArchive(xmlPathFileName);
        return;
    }
    // The save writes the profile and the modules in the background; only once
    // both are on disk are the files worth copying.
    connect(
            mpHost,
            &Host::profileSaveFinished,
            this,
            [this, xmlPathFileName]() {
                writeArchive(xmlPathFileName);
            },
            Qt::SingleShotConnection);
}

void MudletWebExport::writeArchive(const QString& profileXmlPathFileName)
{
    if (!mpHost || mFinished) {
        return;
    }
    // Everything the archive needs from the profile is gathered below; closing
    // the profile while the files are compressed no longer matters.
    disconnect(mHostGone);

    const QString profileName = mpHost->getName();
    const QString homePath = MudletApp::getMudletPath(enums::profileHomePath, profileName);
    const QDir home(homePath);
    const QString archiveCanonical = QFileInfo(mArchivePathFileName).canonicalFilePath();

    // Keyed by the path inside the profile folder; the values are where to read
    // each one from on disk.
    QMap<QString, QString> files;
    QSet<QString> included;
    QDirIterator walk(homePath, QDir::Files | QDir::Hidden | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (walk.hasNext()) {
        const QString path = walk.next();
        const QString relative = home.relativeFilePath(path);
        const QString canonical = walk.fileInfo().canonicalFilePath();
        if (leftBehind(relative) || (!archiveCanonical.isEmpty() && canonical == archiveCanonical)) {
            continue;
        }
        files.insert(relative, path);
        included.insert(canonical);
    }

    const QFileInfo profileXml(profileXmlPathFileName);
    if (!profileXml.isFile()) {
        //: Shown when exporting a profile for Mudlet Web fails because the save it just made cannot be found. %1 is its file name.
        finish(false, tr("The profile's save \"%1\" could not be found.").arg(profileXmlPathFileName));
        return;
    }
    files.insert(qsl("current/%1").arg(profileXml.fileName()), profileXml.absoluteFilePath());

    // Mudlet Web reads a module from "<name>/<its file name>" first. A module
    // installed from an archive needs nothing extra so long as desktop's unpacked
    // copy of it is in the profile folder, which the walk above already took.
    for (const auto& [moduleName, entry] : mpHost->mInstalledModules.asKeyValueRange()) {
        const QString source = entry.value(0);
        if (isArchive(source) && QFileInfo::exists(MudletApp::getMudletPath(enums::profilePackagePathFileName, profileName, moduleName))) {
            continue;
        }
        const QFileInfo sourceInfo(source);
        if (!sourceInfo.isFile()) {
            //: Listed after exporting a profile for Mudlet Web. %1 is the module's name, %2 the file it is installed from.
            mWarnings << tr("Module \"%1\" was left out: its file \"%2\" could not be found.").arg(moduleName, source);
            continue;
        }
        if (included.contains(sourceInfo.canonicalFilePath())) {
            continue;
        }
        files.insert(qsl("%1/%2").arg(moduleName, sourceInfo.fileName()), sourceInfo.absoluteFilePath());
    }

    // The map in memory, not the newest file: it carries whatever was mapped
    // since the last autosave.
    QMap<QString, QByteArray> blobs;
    if (mpHost->mpMap && mpHost->mpMap->mpRoomDB && mpHost->mpMap->mpRoomDB->size() > 0) {
        QByteArray mapBytes;
        QBuffer buffer(&mapBytes);
        buffer.open(QIODevice::WriteOnly);
        QDataStream out(&buffer);
        out.setVersion(QDataStream::Qt_5_12);
        if (mpHost->mpMap->serialize(out)) {
            blobs.insert(qsl("map/%1map.dat").arg(QDateTime::currentDateTime().toString(qsl("yyyy-MM-dd#HH-mm-ss"))), mapBytes);
        } else {
            //: Listed after exporting a profile for Mudlet Web.
            mWarnings << tr("The map could not be written, so it was left out.");
        }
    }

    int zipError = 0;
    zip* archive = zip_open(mArchivePathFileName.toUtf8().constData(), ZIP_CREATE | ZIP_TRUNCATE, &zipError);
    if (!archive) {
        zip_error_t error;
        zip_error_init_with_code(&error, zipError);
        //: Shown when exporting a profile for Mudlet Web fails. %1 is the file being written, %2 the reason.
        const QString message = tr("Could not create \"%1\": %2").arg(mArchivePathFileName, QString::fromUtf8(zip_error_strerror(&error)));
        zip_error_fini(&error);
        finish(false, message);
        return;
    }

    // The profile's own folder at the top, as it sits in desktop's profiles
    // directory, so the archive can be unzipped straight back into one too.
    const QString root = profileName + QLatin1Char('/');
    auto addSource = [archive, &root](const QString& name, zip_source* source) -> QString {
        if (!source) {
            return QString::fromUtf8(zip_strerror(archive));
        }
        if (zip_file_add(archive, (root + name).toUtf8().constData(), source, ZIP_FL_ENC_UTF_8 | ZIP_FL_OVERWRITE) < 0) {
            zip_source_free(source);
            return QString::fromUtf8(zip_strerror(archive));
        }
        return QString();
    };
    for (const auto& [name, path] : files.asKeyValueRange()) {
        if (const QString error = addSource(name, zip_source_file(archive, path.toUtf8().constData(), 0, -1)); !error.isEmpty()) {
            zip_discard(archive);
            //: Shown when exporting a profile for Mudlet Web fails. %1 is the file that could not be added, %2 the reason.
            finish(false, tr("Could not add \"%1\" to the archive: %2").arg(path, error));
            return;
        }
    }
    for (const auto& [name, bytes] : blobs.asKeyValueRange()) {
        if (const QString error = addSource(name, zip_source_buffer(archive, bytes.constData(), static_cast<zip_uint64_t>(bytes.size()), 0)); !error.isEmpty()) {
            zip_discard(archive);
            finish(false, tr("Could not add \"%1\" to the archive: %2").arg(name, error));
            return;
        }
    }

    // libzip reads the files and compresses them only now, which for a profile
    // with sound packs is a long time to hold up the interface. The buffers the
    // map was added from have to outlive that, so the task holds them.
    auto watcher = new QFutureWatcher<QString>(this);
    connect(watcher, &QFutureWatcher<QString>::finished, this, [this, watcher]() {
        const QString error = watcher->result();
        watcher->deleteLater();
        if (!error.isEmpty()) {
            //: Shown when exporting a profile for Mudlet Web fails while the archive is written. %1 is the file being written, %2 the reason.
            finish(false, tr("Could not write \"%1\": %2").arg(mArchivePathFileName, error));
            return;
        }
        finish(true, QString());
    });
    watcher->setFuture(QtConcurrent::run([archive, blobs]() -> QString {
        Q_UNUSED(blobs)
        if (zip_close(archive) == 0) {
            return QString();
        }
        const QString error = QString::fromUtf8(zip_strerror(archive));
        zip_discard(archive);
        return error;
    }));
}

void MudletWebExport::finish(bool ok, const QString& error)
{
    if (mFinished) {
        return;
    }
    mFinished = true;
    emit finished(ok, error, mWarnings);
}
