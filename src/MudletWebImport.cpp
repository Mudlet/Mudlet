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

#include "MudletWebImport.h"

#include "dlgConnectionProfiles.h"
#include "utils.h"

#include <QDataStream>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSaveFile>
#include <QScopeGuard>
#include <QTemporaryDir>

#include <algorithm>
#include <zip.h>

namespace {

struct Entry
{
    zip_uint64_t index = 0;
    zip_uint64_t size = 0;
    QString path;
    QDateTime modified;
};

// Nothing in the archive may land outside the profile it belongs to
bool safeEntryPath(const QString& path)
{
    if (path.startsWith(QLatin1Char('/')) || (path.size() > 1 && path.at(1) == QLatin1Char(':'))) {
        return false;
    }
    const QStringList segments = path.split(QLatin1Char('/'));
    return std::none_of(segments.cbegin(), segments.cend(), [](const QString& segment) {
        return segment.isEmpty() || segment == qsl(".") || segment == qsl("..");
    });
}

bool isSave(const QString& name)
{
    return name.endsWith(qsl(".xml"), Qt::CaseInsensitive);
}

QString usableProfileName(QString name)
{
    name.replace(dlgConnectionProfiles::scmUnusableProfileNameChars, qsl("_"));
    name = name.trimmed();
    // Windows drops a folder name's trailing dots, which would part the profile from its name
    while (name.endsWith(QLatin1Char('.'))) {
        name.chop(1);
    }
    return name;
}

bool extractEntry(zip* archive, const Entry& entry, const QString& destination)
{
    if (!QDir().mkpath(QFileInfo(destination).absolutePath())) {
        return false;
    }
    zip_file* source = zip_fopen_index(archive, entry.index, 0);
    if (!source) {
        return false;
    }
    // Nothing is left at the destination unless the whole entry made it: a cut-off
    // save would otherwise be the one desktop loads
    QSaveFile file(destination);
    bool ok = file.open(QIODevice::WriteOnly);
    zip_uint64_t written = 0;
    char buffer[65536];
    // Read to the end rather than to the size: libzip checks the data's CRC only there
    for (zip_int64_t length = 0; ok && (length = zip_fread(source, buffer, sizeof(buffer))) != 0;) {
        ok = length > 0 && file.write(buffer, length) == length;
        written += ok ? static_cast<zip_uint64_t>(length) : 0;
    }
    // Data that falls short of its stated size ends early rather than failing
    ok = ok && written == entry.size;
    zip_fclose(source);
    if (!ok) {
        file.cancelWriting();
    }
    return file.commit() && ok;
}

QString readProfileItem(const QString& home, const QString& item)
{
    QFile file(qsl("%1/%2").arg(home, item));
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }
    QDataStream stream(&file);
    stream.setVersion(QDataStream::Qt_5_12);
    QString value;
    stream >> value;
    return value;
}

QByteArray readEntry(zip* archive, const Entry& entry)
{
    // Only the connection sidecar is read whole, and it is a few hundred bytes
    constexpr zip_uint64_t limit = 1024 * 1024;
    if (entry.size > limit) {
        return {};
    }
    zip_file* source = zip_fopen_index(archive, entry.index, 0);
    if (!source) {
        return {};
    }
    QByteArray bytes(static_cast<qsizetype>(entry.size), Qt::Uninitialized);
    const zip_int64_t length = zip_fread(source, bytes.data(), entry.size);
    zip_fclose(source);
    return length == static_cast<zip_int64_t>(entry.size) ? bytes : QByteArray();
}

// A connection made before Mudlet Web had a mode is a WebSocket one
bool usesWebSocket(const QJsonObject& sidecar)
{
    const QString mode = sidecar.value(qsl("mode")).toString();
    return mode == qsl("websocket") || (mode.isEmpty() && !sidecar.value(qsl("url")).toString().isEmpty());
}

// The same encoding MudletApp::writeProfileData() gives the files beside current/
bool writeProfileItem(const QString& home, const QString& item, const QString& value)
{
    QSaveFile file(qsl("%1/%2").arg(home, item));
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    QDataStream stream(&file);
    stream.setVersion(QDataStream::Qt_5_12);
    stream << value;
    return stream.status() == QDataStream::Ok && file.commit();
}

// Mudlet Web exports from before it wrote desktop's own connection files carry
// the connection only in its sidecar
// the connection only in its sidecar. Returns how many of its files could not be written.
int writeConnectionFromSidecar(const QString& home, const QJsonObject& sidecar)
{
    const auto checkState = [&sidecar](const char* key) {
        return QString::number(sidecar.value(QLatin1String(key)).toBool() ? Qt::Checked : Qt::Unchecked);
    };
    QList<std::pair<QString, QString>> items;
    if (!usesWebSocket(sidecar)) {
        if (const QString host = sidecar.value(qsl("host")).toString(); !host.isEmpty()) {
            items.append({qsl("url"), host});
        }
        if (const int port = sidecar.value(qsl("port")).toInt(); port > 0 && port <= 65535) {
            items.append({qsl("port"), QString::number(port)});
        }
    }
    items.append({qsl("login"), sidecar.value(qsl("charLoginAccount")).toString()});
    items.append({qsl("description"), sidecar.value(qsl("description")).toString()});
    items.append({qsl("autologin"), checkState("autoReconnect")});
    items.append({qsl("autoreconnect"), checkState("reconnectOnDrop")});
    items.append({qsl("ssl_tsl"), checkState("tls")});
    return static_cast<int>(std::count_if(items.cbegin(), items.cend(), [&home](const auto& item) {
        return !writeProfileItem(home, item.first, item.second);
    }));
}

// Desktop loads the save and map with the newest modification time, and
// extracting gives them all about the same one. Ordered by when the archive says
// each was last changed, and where it can't tell them apart - Mudlet Web dates
// a whole export alike - by their time-stamped names; a save named by hand counts as older.
void putNewestLast(const QString& folder, const QStringList& filters, const QHash<QString, QDateTime>& modified)
{
    static const QRegularExpression stamped(qsl(R"(^\d{4}-\d{2}-\d{2}#\d{2}-\d{2}-\d{2})"));
    QFileInfoList files = QDir(folder).entryInfoList(filters, QDir::Files);
    std::sort(files.begin(), files.end(), [&modified](const QFileInfo& a, const QFileInfo& b) {
        const QDateTime aModified = modified.value(a.absoluteFilePath());
        const QDateTime bModified = modified.value(b.absoluteFilePath());
        if (aModified != bModified) {
            return aModified < bModified;
        }
        const bool aStamped = stamped.match(a.fileName()).hasMatch();
        const bool bStamped = stamped.match(b.fileName()).hasMatch();
        return aStamped != bStamped ? bStamped : a.fileName() < b.fileName();
    });
    const QDateTime first = QDateTime::currentDateTime().addSecs(-files.size());
    for (qsizetype i = 0; i < files.size(); ++i) {
        QFile file(files.at(i).absoluteFilePath());
        if (file.open(QIODevice::ReadWrite)) {
            file.setFileTime(first.addSecs(i), QFileDevice::FileModificationTime);
        }
    }
}

QString freeName(const QString& wanted, const QStringList& takenLowerCase)
{
    QString name = wanted;
    for (int n = 2; takenLowerCase.contains(name.toLower()); ++n) {
        name = qsl("%1 (%2)").arg(wanted, QString::number(n));
    }
    return name;
}

} // namespace

MudletWebImport::Result MudletWebImport::importArchive(const QString& archivePathFileName, const QString& profilesPath)
{
    Result result;
    int zipError = 0;
    zip* archive = zip_open(archivePathFileName.toUtf8().constData(), ZIP_RDONLY, &zipError);
    if (!archive) {
        zip_error_t error;
        zip_error_init_with_code(&error, zipError);
        //: Shown when importing profiles fails. %1 is the file chosen, %2 the reason.
        result.error = tr("Could not open \"%1\": %2").arg(QFileInfo(archivePathFileName).fileName(), QString::fromUtf8(zip_error_strerror(&error)));
        zip_error_fini(&error);
        return result;
    }
    auto closeArchive = qScopeGuard([archive]() {
        zip_discard(archive);
    });

    QList<Entry> entries;
    bool unsafeEntries = false;
    for (zip_int64_t i = 0, total = zip_get_num_entries(archive, 0); i < total; ++i) {
        zip_stat_t stat;
        constexpr zip_uint64_t needed = ZIP_STAT_NAME | ZIP_STAT_SIZE;
        if (zip_stat_index(archive, static_cast<zip_uint64_t>(i), 0, &stat) != 0 || (stat.valid & needed) != needed) {
            continue;
        }
        const QString path = QString::fromUtf8(stat.name).replace(QLatin1Char('\\'), QLatin1Char('/'));
        if (path.endsWith(QLatin1Char('/'))) {
            continue;
        }
        if (!safeEntryPath(path)) {
            unsafeEntries = true;
            continue;
        }
        const QDateTime modified = (stat.valid & ZIP_STAT_MTIME) ? QDateTime::fromSecsSinceEpoch(stat.mtime) : QDateTime();
        entries.append({static_cast<zip_uint64_t>(i), stat.size, path, modified});
    }

    // A profile is a folder with a save in current/: one per folder at the top,
    // as both Mudlet Web and desktop export them, or the whole archive when its
    // own top level is the profile
    QStringList roots;
    bool bare = false;
    for (const auto& entry : std::as_const(entries)) {
        const QStringList segments = entry.path.split(QLatin1Char('/'));
        if (segments.size() == 2 && segments.at(0) == qsl("current") && isSave(segments.at(1))) {
            bare = true;
        } else if (segments.size() == 3 && segments.at(1) == qsl("current") && isSave(segments.at(2)) && !roots.contains(segments.at(0))) {
            roots.append(segments.at(0));
        }
    }
    if (bare) {
        roots = QStringList{QString()};
    }
    roots.sort();
    if (roots.isEmpty()) {
        //: Shown when importing profiles finds nothing to import. %1 is the file chosen.
        result.error = tr("\"%1\" holds no Mudlet profile: none of its folders has a save in a \"current\" folder.").arg(QFileInfo(archivePathFileName).fileName());
        return result;
    }
    if (unsafeEntries) {
        //: Listed after importing profiles.
        result.warnings.append(tr("Some files in the archive were left out: their paths lead outside the profile."));
    }

    // A fresh install makes it only when its first profile is saved
    if (!QDir().mkpath(profilesPath)) {
        //: Shown when importing profiles fails. %1 is the folder that holds every profile.
        result.error = tr("Could not create the profiles folder \"%1\".").arg(QDir::toNativeSeparators(profilesPath));
        return result;
    }

    // Staged beside the profiles folder rather than in it, where the profile list
    // would show a half-written profile, yet on the same disk so it moves in whole:
    // a profiles folder that is a link may lead to another disk
    QTemporaryDir staging(qsl("%1/mudlet-web-import-XXXXXX").arg(QFileInfo(QFileInfo(profilesPath).canonicalFilePath()).absolutePath()));
    if (!staging.isValid()) {
        //: Shown when importing profiles fails. %1 is the reason.
        result.error = tr("Could not make room to unpack the profiles: %1").arg(staging.errorString());
        return result;
    }

    QStringList taken;
    for (const auto& existing : QDir(profilesPath).entryList(QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden)) {
        taken.append(existing.toLower());
    }

    for (const auto& root : std::as_const(roots)) {
        QString wanted = usableProfileName(root.isEmpty() ? QFileInfo(archivePathFileName).completeBaseName() : root);
        if (wanted.isEmpty()) {
            //: The name given to an imported profile whose own name can't be a folder name
            wanted = tr("Mudlet Web profile");
        }
        const QString name = freeName(wanted, taken);
        const QString home = qsl("%1/%2").arg(staging.path(), name);
        const QString prefix = root.isEmpty() ? QString() : root + QLatin1Char('/');

        QByteArray sidecar;
        int unwritten = 0;
        QHash<QString, QDateTime> modified;
        for (const auto& entry : std::as_const(entries)) {
            if (!entry.path.startsWith(prefix)) {
                continue;
            }
            QString path = entry.path.mid(prefix.size());
            const QString top = path.section(QLatin1Char('/'), 0, 0);
            // Mudlet Web's bookkeeping, meaningless here but for the connection it describes;
            // .mudix is what it was called before Mudlet Web had its name
            if (top == qsl(".mudlet") || top == qsl(".mudix")) {
                if (path.section(QLatin1Char('/'), 1) == qsl("connection.json") && (sidecar.isEmpty() || top == qsl(".mudlet"))) {
                    sidecar = readEntry(archive, entry);
                }
                continue;
            }
            if (top == qsl("logs")) {
                path = qsl("log") + path.mid(top.size());
            }
            const QString destination = qsl("%1/%2").arg(home, path);
            if (extractEntry(archive, entry, destination)) {
                modified.insert(QFileInfo(destination).absoluteFilePath(), entry.modified);
            } else {
                ++unwritten;
            }
        }

        if (QDir(qsl("%1/current").arg(home)).entryList({qsl("*.[xX][mM][lL]")}, QDir::Files).isEmpty()) {
            //: Listed after importing profiles. %1 is a profile's name.
            result.warnings.append(tr("\"%1\" was not added: its save could not be written.").arg(name));
            continue;
        }
        const QJsonObject connection = QJsonDocument::fromJson(sidecar).object();
        if (!QFileInfo::exists(qsl("%1/url").arg(home)) && !connection.isEmpty()) {
            unwritten += writeConnectionFromSidecar(home, connection);
        }
        putNewestLast(qsl("%1/current").arg(home), {qsl("*.[xX][mM][lL]")}, modified);
        putNewestLast(qsl("%1/map").arg(home), {qsl("*.[dD][aA][tT]"), qsl("*.[jJ][sS][oO][nN]")}, modified);

        const QString target = qsl("%1/%2").arg(profilesPath, name);
        if (QFileInfo::exists(target) || !QDir().rename(home, target)) {
            //: Listed after importing profiles. %1 is a profile's name.
            result.warnings.append(tr("\"%1\" could not be added to your profiles.").arg(name));
            continue;
        }
        taken.append(name.toLower());
        result.profiles.append(name);

        if (unwritten) {
            //: Listed after importing profiles. %1 is a profile's name.
            result.warnings.append(tr("%n file(s) of \"%1\" could not be written and were left out.", nullptr, unwritten).arg(name));
        }
        if (readProfileItem(target, qsl("url")).trimmed().isEmpty()) {
            if (usesWebSocket(connection)) {
                //: Listed after importing profiles. %1 is a profile's name. Desktop Mudlet can't connect to a WebSocket (ws:// or wss://) address.
                result.warnings.append(
                        tr("\"%1\" connected to its game through a WebSocket address, which desktop Mudlet can't use: enter the game's server address and port before you connect.").arg(name));
            } else {
                //: Listed after importing profiles. %1 is a profile's name.
                result.warnings.append(tr("\"%1\" has no server address: enter the game's server address and port before you connect.").arg(name));
            }
        }
    }

    if (result.profiles.isEmpty()) {
        //: Shown when importing profiles fails. %1 lists what went wrong with each.
        result.error = tr("No profile could be imported.\n%1").arg(result.warnings.join(QLatin1Char('\n')));
        result.warnings.clear();
    }
    return result;
}

QString MudletWebImport::locateModuleFile(const QString& profileHome, const QString& module, const QString& filePath)
{
    const QFileInfo stored(filePath);
    if (filePath.isEmpty() || (stored.isAbsolute() && stored.exists())) {
        return filePath;
    }
    // Mudlet Web lists a module by its place inside the profile, and another
    // computer by a path that only exists there - either way the file came along
    const QString portable = QString(filePath).replace(QLatin1Char('\\'), QLatin1Char('/'));
    QStringList candidates;
    if (stored.isRelative()) {
        candidates.append(qsl("%1/%2").arg(profileHome, portable));
    }
    candidates.append(qsl("%1/%2/%3").arg(profileHome, module, portable.section(QLatin1Char('/'), -1)));
    for (const auto& candidate : std::as_const(candidates)) {
        if (QFileInfo(candidate).isFile()) {
            return QDir::cleanPath(candidate);
        }
    }
    return filePath;
}
