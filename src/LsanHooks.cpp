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

#include "LsanSuppressions.h"

// Inert unless the LeakSanitizer runtime is linked in. They cannot be guarded
// with an "is ASAN on" macro check: Mudlet only applies -fsanitize=address at
// link time, so no compiler macro is set, and Qt's qcompilerdetection.h shims
// __has_feature to 0 on GCC anyway. Built as their own OBJECT library, see
// src/CMakeLists.txt.

// Keeps third-party noise (GPU drivers, font stack) out of the leak reports
// that testing/PTB builds show users, with no LSAN_OPTIONS set at runtime
extern "C" const char* __lsan_default_suppressions()
{
    return mudletLsanSuppressions;
}

// print_suppressions=0: without it, LeakSanitizer appends a "Suppressions used"
// summary to every clean exit, which reads like an error to users.
//
// intercept_tls_get_addr=0: sanitizer-enabled builds run LeakSanitizer when the
// user quits, and gcc-10's - the compiler this AppImage is built with - guesses
// how glibc allocated a dynamic TLS block from `(tls_beg % 4096) == 16`, true
// for any malloc'd block at that alignment since glibc 2.25. It then reads two
// words of unrelated memory as that block's address and length, and the tracer
// segfaults scanning them (google/sanitizers#1322, #9809), so quitting ends in
// "LeakSanitizer has encountered a fatal error" and a non-zero exit. Turning the
// interception off skips the recording, at the cost of not scanning dynamic TLS
// for roots. GCC 14 and LLVM 17 fixed the guess.
extern "C" const char* __lsan_default_options()
{
    return "print_suppressions=0:intercept_tls_get_addr=0";
}
