/***************************************************************************
 *   Copyright (C) 2013-2026 by Stephen Lyons - slysven@virginmedia.com    *
 *   Copyright (C) 2016-2018 by Ian Adkins - ieadkins@gmail.com            *
 *   Copyright (C) 2011-2021 by Vadim Peretokin - vperetokin@gmail.com     *
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

#include "utils.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QMap>

#include <zip.h>

// We are now using code that won't work with really old versions of libzip;
// some of the error handling was improved in 1.0 . Unfortunately libzip 1.7.0
// (and one or two other recent versions) forgot to include the version defines
// and thus broke a test depending on them:
#if defined(LIBZIP_VERSION_MAJOR) && (LIBZIP_VERSION_MAJOR < 1)
#error Mudlet requires a version of libzip of at least 1.0
#endif

bool utils::unzip(const QString& archivePath, const QString& destination, const QDir& tmpDir)
{
    int err = 0;
    //from: https://gist.github.com/mobius/1759816
    struct zip_stat zs;
    struct zip_file* zf;
    zip_uint64_t bytesRead = 0;
    zip* archive = zip_open(archivePath.toUtf8().constData(), 0, &err);
    if (!archive) {
        zip_error_t error;
        zip_error_init_with_code(&error, err);
        qWarning().noquote().nospace() << "utils::unzip(\"" << archivePath << "\", \"" << destination << "\", \"" << tmpDir.absolutePath() << "\") WARNING - failed to unzip file, error: \""
                                       << zip_error_strerror(&error) << "\"";
        zip_error_fini(&error);
        return false;
    }

    // Gather the needed directories first rather than relying on (zero length) entries ending in '/',
    // which some archive building libraries omit.
    QMap<QString, QString> directoriesNeededMap;
    //   Key is: relative path stored in archive
    // Value is: absolute path needed when extracting files
    for (zip_int64_t i = 0, total = zip_get_num_entries(archive, 0); i < total; ++i) {
        // Only fields zs.valid marks as filled may be read, so skip an entry libzip couldn't name rather
        // than read the previous entry's leftovers
        if (!zip_stat_index(archive, static_cast<zip_uint64_t>(i), 0, &zs) && (zs.valid & ZIP_STAT_NAME)) {
            const QString entryInArchive(zs.name);
            const QString pathInArchive(entryInArchive.section(qsl("/"), 0, -2));
            if (entryInArchive.endsWith(QLatin1Char('/'))) {
                if (!directoriesNeededMap.contains(pathInArchive)) {
                    directoriesNeededMap.insert(pathInArchive, pathInArchive);
                }
            } else {
                if (!pathInArchive.isEmpty() && !directoriesNeededMap.contains(pathInArchive)) {
                    directoriesNeededMap.insert(pathInArchive, pathInArchive);
                }
            }
        }
    }

    QMapIterator<QString, QString> itPath(directoriesNeededMap);
    while (itPath.hasNext()) {
        itPath.next();
        const QString folderToCreate = qsl("%1%2").arg(destination, itPath.value());
        if (!tmpDir.exists(folderToCreate)) {
            if (!tmpDir.mkpath(folderToCreate)) {
                zip_close(archive);
                return false;
            }
            tmpDir.refresh();
        }
    }

    for (zip_int64_t i = 0, total = zip_get_num_entries(archive, 0); i < total; ++i) {
        // Unlike above, a failure can't be skipped: zs would still hold the previous entry's name and size,
        // so that file would be extracted again, from this entry's data. An archive libzip can't describe
        // isn't extractable.
        constexpr zip_uint64_t neededFields = ZIP_STAT_NAME | ZIP_STAT_SIZE;
        if (zip_stat_index(archive, static_cast<zip_uint64_t>(i), 0, &zs) || (zs.valid & neededFields) != neededFields) {
            zip_close(archive);
            return false;
        }
        const QString entryInArchive(zs.name);
        if (!entryInArchive.endsWith(QLatin1Char('/'))) {
            zf = zip_fopen_index(archive, static_cast<zip_uint64_t>(i), 0);
            if (!zf) {
                zip_close(archive);
                return false;
            }

            QFile fd(qsl("%1%2").arg(destination, entryInArchive));

            if (!fd.open(QIODevice::ReadWrite | QIODevice::Truncate)) {
                zip_fclose(zf);
                zip_close(archive);
                return false;
            }

            bytesRead = 0;
            zip_uint64_t const bytesExpected = zs.size;
            while (bytesRead < bytesExpected && fd.error() == QFileDevice::NoError) {
                char buf[4096];
                zip_int64_t const len = zip_fread(zf, buf, sizeof(buf));
                // zip_fread() reports end of data as 0, not an error, so a declared size larger than the data
                // would spin forever on unzipAsync()'s worker thread, where nothing can interrupt it
                if (len <= 0) {
                    fd.close();
                    zip_fclose(zf);
                    zip_close(archive);
                    return false;
                }

                if (fd.write(buf, len) == -1) {
                    fd.close();
                    zip_fclose(zf);
                    zip_close(archive);
                    return false;
                }
                bytesRead += static_cast<zip_uint64_t>(len);
            }
            fd.close();
            zip_fclose(zf);
        }
    }

    err = zip_close(archive);
    if (err) {
        zip_error_t* error = zip_get_error(archive);
        qWarning().noquote().nospace() << "utils::unzip(\"" << archivePath << "\", \"" << destination << "\", \"" << tmpDir.absolutePath() << "\") Warning - " << zip_error_strerror(error);
        zip_error_fini(error);
        zip_discard(archive);
        return false;
    }

    return true;
}
