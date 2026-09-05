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

    // We now scan for directories first, and gather needed ones first, not
    // just relying on (zero length) archive entries ending in '/' as some
    // (possibly broken) archive building libraries seem to forget to
    // include them.
    QMap<QString, QString> directoriesNeededMap;
    //   Key is: relative path stored in archive
    // Value is: absolute path needed when extracting files
    for (zip_int64_t i = 0, total = zip_get_num_entries(archive, 0); i < total; ++i) {
        if (!zip_stat_index(archive, static_cast<zip_uint64_t>(i), 0, &zs)) {
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

    // Now create the needed directories:
    QMapIterator<QString, QString> itPath(directoriesNeededMap);
    while (itPath.hasNext()) {
        itPath.next();
        const QString folderToCreate = qsl("%1%2").arg(destination, itPath.value());
        if (!tmpDir.exists(folderToCreate)) {
            if (!tmpDir.mkpath(folderToCreate)) {
                zip_close(archive);
                return false; // Abort reading rest of archive
            }
            tmpDir.refresh();
        }
    }

    // Now extract the files
    for (zip_int64_t i = 0, total = zip_get_num_entries(archive, 0); i < total; ++i) {
        // No need to check return value as we've already done it first time
        zip_stat_index(archive, static_cast<zip_uint64_t>(i), 0, &zs);
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
                char buf[4096]; // Was 100 but that seems unduly stingy...!
                zip_int64_t const len = zip_fread(zf, buf, sizeof(buf));
                if (len < 0) {
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
