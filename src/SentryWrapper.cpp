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
#include <QStandardPaths>
#include "sentry.h"
#endif

#ifdef Q_OS_WIN
#include <windows.h>
#elif defined(Q_OS_MAC)
#include <mach-o/dyld.h>
#include <limits.h>
#else
#include <unistd.h>
#include <limits.h>
#endif

#include <string>
#include <cstdlib>
#include <algorithm>

#include "SentryWrapper.h"

// Initializes Sentry options for crash/error reporting.
// Crashes are first stored in a local cache folder, then automatically sent.
//
// Expected cache locations:
//   Linux   : ~/.cache/mudlet/sentry
//   macOS   : ~/Library/Caches/mudlet/sentry
//   Windows : C:\Users\...\AppData\Local\Cache\Mudlet\sentry
void initSentry()
{
    #ifdef WITH_SENTRY
        sentry_options_t*   options = sentry_options_new();
        std::string         runtimeAppDir = getExeDir();
        QString             path = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/mudlet/sentry";

        if (!options) {
            return;
        }
        sentry_options_set_database_path(options, path.toUtf8().constData());
        sentry_options_set_release(options, "mudlet@" APP_VERSION);
        sentry_options_set_handler_path(options, makeExecutablePath(runtimeAppDir, "crashpad_handler").c_str());
        sentry_options_set_external_crash_reporter_path(options, makeExecutablePath(runtimeAppDir, "MudletCrashReporter").c_str());

        sentry_init(options);
    #endif
}

std::string makeExecutablePath(const std::string& dir, const std::string& name)
{
    #ifdef Q_OS_WIN
        return dir + "/" + name + ".exe";
    #else
        return dir + "/" + name;
    #endif
}

// Returns the directory containing the current executable
std::string getExeDir()
{
#if defined(Q_OS_WIN)
    char path[MAX_PATH];
    GetModuleFileNameA(NULL, path, MAX_PATH);
    std::string exePath(path);
    size_t pos = exePath.find_last_of("\\/");
    std::string dir = exePath.substr(0, pos);
    std::replace(dir.begin(), dir.end(), '\\', '/');

    return dir;
#elif defined(Q_OS_MAC)
    char path[PATH_MAX];
    uint32_t size = sizeof(path);

    if (_NSGetExecutablePath(path, &size) != 0) {
        return ".";
    }
    std::string exePath(path);
    size_t pos = exePath.find_last_of("/");
    if (pos == std::string::npos) {
        return exePath;
    }
    return exePath.substr(0, pos);
#else
    char result[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", result, PATH_MAX); // Flawfinder: ignore

    if (count <= 0) {
        return ".";
    }
    std::string exePath(result, count);
    size_t pos = exePath.find_last_of("/");
    if (pos == std::string::npos) {
        return exePath;
    }
    return exePath.substr(0, pos);
#endif
}

void crashIfRequested()
{
    const char* environmentVariable = std::getenv("MUDLET_CRASH_TEST");

     if (environmentVariable && *environmentVariable == '1') {
        int* p = nullptr;
        *p = 42;
    }
}
