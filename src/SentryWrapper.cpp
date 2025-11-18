/***************************************************************************
 *   Copyright (C) 2025 by Nicolas Keita - nicolaskeita2@gmail.com         *
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

#ifdef WITH_SENTRY
    #include <QtCore/qscopeguard.h>
    #include <QStandardPaths>
    #include <QCoreApplication>
    #include "sentry.h"
#endif

#include <string>
#include "SentryWrapper.h"

/**
 * Initializes Sentry options for crash/error reporting,
 * Crashes are first stored in a local cache folder, then automatically sent.
 *
 * Expected cache locations:
 *   Linux   : ~/.cache/mudlet/sentry
 *   macOS   : ~/Library/Caches/mudlet/sentry
 *   Windows : C:\Users\...\AppData\Local\Cache\Mudlet\sentry
 */
void initSentry()
{
    #ifdef WITH_SENTRY
        sentry_options_t* options = sentry_options_new();

        if (!options) {
            return;
        }
        sentry_options_set_dsn(options, SENTRY_DSN);
        QString path = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/mudlet/sentry";
        sentry_options_set_database_path(options, path.toUtf8().constData());
        sentry_options_set_release(options, "mudlet@" APP_VERSION);
        sentry_options_set_handler_path(options, makeExecutablePath(APP_DIR_PATH, "crashpad_handler").c_str());
        sentry_options_set_external_crash_reporter_path(options, makeExecutablePath(APP_DIR_PATH, "MudletCrashReporter").c_str());

        sentry_init(options);
    #endif
}

std::string makeExecutablePath(const std::string& dir, const std::string& name) {
    #ifdef _WIN32
        return dir + "/" + name + ".exe";
    #else
        return dir + "/" + name;
    #endif
}
