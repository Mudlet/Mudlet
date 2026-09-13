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
// intercept_tls_get_addr=0: sanitizer-enabled builds - the testing and PTB ones
// users download - run LeakSanitizer when the user quits. Up to GCC 13 and LLVM
// 16, the sanitizer runtime's __tls_get_addr interceptor decides a dynamic TLS
// block carries a pre-2.25 glibc header when `tls_beg % 4096` equals
// sizeof(Glibc_2_19_tls_header) (16 on 64-bit), and reads the two words before
// it as that block's address and length. Since glibc 2.25 there is no such
// header, and a malloc'd block lands on that offset by coincidence, so the
// recorded range is garbage; LeakSanitizer's tracer then segfaults scanning it
// and quitting ends in "LeakSanitizer has encountered a fatal error" and a
// non-zero exit (google/sanitizers#1322, #9809). Turning the interception off
// skips the recording, at the cost of not scanning dynamic TLS for roots.
//
// Set unconditionally rather than only for the affected runtimes: there is no
// macro that names the sanitizer runtime here (see above), and on a fixed one
// the flag costs only that scanning. The CI leak leg sets it through
// ASAN_OPTIONS as well - see test/functional_tests/CMakeLists.txt.
extern "C" const char* __lsan_default_options()
{
    return "print_suppressions=0:intercept_tls_get_addr=0";
}
