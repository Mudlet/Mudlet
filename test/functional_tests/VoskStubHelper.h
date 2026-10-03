#ifndef MUDLET_VOSKSTUBHELPER_H
#define MUDLET_VOSKSTUBHELPER_H

/***************************************************************************
 *   Copyright (C) 2026 by Mike Conley - mike.conley@stickmud.com          *
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

// The stand-in libvosk built by test/functional_tests/CMakeLists.txt, installed
// where VoskRecognizer looks for a library so the code below its library guard
// can be reached on a machine with no speech engine - which is every CI runner.

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLibrary>

#include "VoskRecognizer.h"
#include "utils.h"

namespace VoskStub {

// Where install() puts the copy VoskRecognizer actually loads.
inline QString installedPath()
{
    return QDir(VoskRecognizer::userLibraryPath()).filePath(QFileInfo(qsl(MUDLET_VOSK_STUB_LIBRARY)).fileName());
}

// Whether a libvosk on the loader's own path would be found before the stub.
// loadVoskLibrary() asks QLibrary for the bare name "vosk" and only falls back
// to librarySearchPaths() when that fails, so on such a machine a case that
// installs the stub is really driving the system engine.
//
// Ask this before anything has loaded a stub: dlopen() and LoadLibrary() both
// answer a bare name from what is already mapped, so asking afterwards would
// report the stub as the system engine.
inline bool systemEngineWins()
{
    QLibrary bare(qsl("vosk"));
    const bool loaded = bare.load();
    if (loaded) {
        bare.unload();
    }
    return loaded;
}

// Puts the stand-in engine where librarySearchPaths() looks first. Returns
// false when the copy could not be made, which a case reports rather than
// skips - the stub is built by this project, so its absence is a build fault
// and not an environment one.
inline bool install()
{
    const QString destination = installedPath();
    if (!QDir().mkpath(VoskRecognizer::userLibraryPath())) {
        return false;
    }
    QFile::remove(destination);
    // Fresh probe: libraryAvailable() caches, and an earlier case in this
    // shared process may have answered "no" before the file existed.
    VoskRecognizer::resetLibraryLoadState();
    VoskRecognizer::unloadLibraryByRequest(false);
    if (!QFile::copy(qsl(MUDLET_VOSK_STUB_LIBRARY), destination)) {
        return false;
    }
    // A copy that loads but exports nothing is the failure worth catching here
    // rather than three assertions later: every case would then be testing a
    // recognizer that never got past its library guard.
    //
    // Every load below is given back before this returns. QLibrary refcounts
    // and its destructor does not unload, so a probe left standing keeps the
    // module mapped however many times the recognizer unloads its own handle -
    // and remove() then silently fails on Windows, which refuses to delete a
    // mapped module, leaving the next case's copy to find the old file in place.
    QLibrary installed(destination);
    if (!installed.load() || !installed.resolve("vosk_recognizer_set_words")) {
        installed.unload();
        return false;
    }
    // Its counters are process-global and live as long as the image stays
    // mapped, so they are zeroed here rather than trusted to be zero. Without
    // this, a case asserting "no null handles" would be asserting that no
    // earlier case caused one either, and would start failing when the cases
    // are reordered.
    using resetFn = void (*)();
    auto* reset = reinterpret_cast<resetFn>(installed.resolve("voskStubReset"));
    if (!reset) {
        installed.unload();
        return false;
    }
    reset();
    installed.unload();
    return true;
}

// Takes the stub back out. Call it after VoskRecognizer::resetLibraryLoadState()
// so the file is not still mapped when it goes; Windows refuses to delete a
// module that is.
inline void remove()
{
    QFile::remove(installedPath());
}

// The stub's own record of how many null recognizer handles it was handed,
// resolved by name because the library is loaded by path rather than linked.
// -1 when the counter cannot be read.
//
// From the installed copy, not the one in the build tree: they are two files,
// so the loader maps them as two images with a counter each, and reading the
// build copy's would answer 0 however many nulls the recognizer handed the one
// it loaded - an assertion that could never fail.
inline int nullHandleCalls()
{
    QLibrary stub(installedPath());
    using countFn = int (*)();
    auto* counter = reinterpret_cast<countFn>(stub.resolve("voskStubNullHandleCalls"));
    const int calls = counter ? counter() : -1;
    // resolve() mapped the module to answer, and nothing else here would give
    // that back. The recognizer holds its own handle, so the counters this
    // just read stay put.
    stub.unload();
    return calls;
}

} // namespace VoskStub

#endif // MUDLET_VOSKSTUBHELPER_H
